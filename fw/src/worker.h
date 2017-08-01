#pragma once

#include <functional>
#include <memory>

#include "freertos/queue.h"

struct Worker {
    using Job = std::function< void() >;
    Worker( int maxJobs = 10 )
        : _queue( xQueueCreate( maxJobs, sizeof( Job * ) ) )
    { }

    // ToDo: Proper destructor

    void loop() {
        while( true ) {
            Job* jobPtr;
            xQueueReceive( _queue, &jobPtr, portMAX_DELAY );
            std::unique_ptr< Job > job{ jobPtr };
            ( *job )();
        }
    }

    void run() {
        Job* jobPtr;
        if ( xQueueReceive( _queue, &jobPtr, 0 ) == pdFALSE )
            return;
        std::unique_ptr< Job > job{ jobPtr };
        ( *job )();
    }

    template < typename F >
    void push( F f ) {
        auto job = std::make_unique< Job >( f );
        auto* jobPtr = job.get();
        if ( xQueueSend( _queue, &jobPtr, 0 ) == pdTRUE )
            job.release();
    }

    template < typename F >
    void pushFromIsr( F f ) {
        auto job = std::make_unique< Job >( f );
        auto* jobPtr = job.get();
        if ( xQueueSendFromISR( _queue, &jobPtr, 0 ) == pdTRUE )
            job.release();
    }

    QueueHandle_t _queue;
};