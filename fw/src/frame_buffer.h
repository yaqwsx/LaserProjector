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
          _queue( xQueueCreate( maxCapacity, sizeof( Frame* ) ) )
    {}

    ImgPoint nextFromIsr() {
        if ( !_currentFrame ) {
            _popNextFrameFromIsr();
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
                _popNextFrameFromIsr();
        }
        return p;
    }

    void _popNextFrameFromIsr() {
        Frame* nextFramePtr = nullptr;
        if ( xQueueReceiveFromISR( _queue, &nextFramePtr, 0 ) == pdFALSE )
            return;
        _currentFrame.reset( nextFramePtr );
    }

    bool push( Frame&& f ) {
        auto frame = std::make_unique< Frame >( std::move( f ) );
        auto* framePtr = frame.get();
        if ( xQueueSend( _queue, &framePtr, 0 ) == pdTRUE ) {
            frame.release();
            return true;
        }
        return false;
    }

    std::unique_ptr< Frame > _currentFrame;
    int _progress;
    QueueHandle_t _queue;
};