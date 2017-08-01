#pragma once

#include <vector>

#include "packet.h"
#include "img_point.h"

#include "freertos/queue.h"

// Frame internally operates on top of received packet in order to minimize
// number of copies made
struct Frame {
    Frame ( Packet&& packet )
        : _packet( packet )
    {}

    uint16_t& repetition() {
        return _packet._as< uint16_t >( Packet::DataOffset );
    }

    ImgPoint operator[]( int idx ) {
        return _packet.get< ImgPoint >( 2 + idx * sizeof( ImgPoint ) );
    }

    int size() {
        return ( _packet.size() - 2 ) / sizeof( ImgPoint );
    }

    Packet _packet;
};

struct FrameBuffer {
    FrameBuffer( int maxCapacity = 10 )
        : _progress( 0 ),
          _frameQueue( xQueueCreate( maxCapacity, sizeof( Frame* ) ) ),
          _preprocessedQueue( xQueueCreate( 32, sizeof( ImgPoint ) ) )
    {}

    template < typename F >
    void start( F f ) {
        _transform = f;
        xTaskCreate( run, "FraBuf", 4096, this, 5, nullptr );
    }

    static void run( void* arg ) {
        auto& self = *reinterpret_cast< FrameBuffer* >( arg );
        self._run();
    }

    void _run() {
        while ( true ) {
            ImgPoint p = next();
            p = _transform( p );
            xQueueSend( _preprocessedQueue, &p, portMAX_DELAY );
        }
    }

    ImgPoint IRAM_ATTR nextFromIsr() {
        ImgPoint p;
        auto ret = xQueueReceiveFromISR( _preprocessedQueue, &p, nullptr );
        assert( ret == pdTRUE );
        return p;
    }

    ImgPoint next() {
        if ( !_currentFrame ) {
            _popNextFrame();
            if ( !_currentFrame )
                return { 0, 0, 0 };
        }
        ImgPoint p = ( *_currentFrame )[ _progress ];
        _progress++;
        if ( _progress >= _currentFrame->size() ) {
            _progress = 0;
            if ( _currentFrame->repetition() )
                _currentFrame->repetition()--;
            if ( _currentFrame->repetition() == 0 )
                _popNextFrame();
        }
        return p;
    }

    void _popNextFrame() {
        Frame* nextFramePtr = nullptr;
        if ( xQueueReceive( _frameQueue, &nextFramePtr, 0 ) == pdFALSE )
            return;
        _currentFrame.reset( nextFramePtr );
    }

    bool push( Frame&& f ) {
        auto frame = std::make_unique< Frame >( std::move( f ) );
        auto* framePtr = frame.get();
        if ( xQueueSend( _frameQueue, &framePtr, 0 ) == pdTRUE ) {
            frame.release();
            return true;
        }
        return false;
    }

    std::unique_ptr< Frame > _currentFrame;
    int _progress;
    QueueHandle_t _frameQueue;
    QueueHandle_t _preprocessedQueue;
    std::function< ImgPoint( ImgPoint ) > _transform;
};