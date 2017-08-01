#pragma once

extern "C" {
    #include <soc/timer_group_struct.h>
    #include <soc/soc.h>
}

#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>

#include <functional>

static inline void DispatcherIsr( void *arg );

struct Dispatcher {

    Dispatcher( int frequency, gpio_num_t debugPin )
        : _dPin( debugPin )
    {
        timer_config_t config;
        config.alarm_en = 1;
        config.auto_reload = 1;
        config.counter_dir = TIMER_COUNT_UP;
        config.divider = 1;
        config.intr_type = TIMER_INTR_LEVEL;
        config.counter_en = TIMER_PAUSE;

        timer_init( TIMER_GROUP_0, TIMER_1, &config );
        timer_pause( TIMER_GROUP_0, TIMER_1 );
        timer_set_alarm_value( TIMER_GROUP_0, TIMER_1, APB_CLK_FREQ / frequency / 2 );
        timer_enable_intr( TIMER_GROUP_0, TIMER_1 );
        timer_isr_register( TIMER_GROUP_0, TIMER_1, DispatcherIsr,
            reinterpret_cast< void * >( this ), ESP_INTR_FLAG_IRAM, nullptr );
    }

    ~Dispatcher() {
        // ToDo: Clear interrupt handler
        stop();
    }

    template < typename F >
    void start( F f ) {
        _job = f;
        timer_set_counter_value( TIMER_GROUP_0, TIMER_1, 0 );
        timer_start( TIMER_GROUP_0, TIMER_1 );

        gpio_set_direction( _dPin, GPIO_MODE_OUTPUT );
        gpio_set_level( _dPin, 0 );
    }

    void stop() {
        timer_pause( TIMER_GROUP_0, TIMER_1 );
    }

    gpio_num_t _dPin;
    std::function< void() > _job;
};

static inline void IRAM_ATTR DispatcherIsr( void *arg ) {
    auto& self = *reinterpret_cast< Dispatcher *>( arg );
    gpio_set_level( self._dPin, 1 );
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    self._job();
    TIMERG0.hw_timer[ TIMER_1 ].update = 1;
    TIMERG0.int_clr_timers.t1 = 1;
    TIMERG0.hw_timer[ TIMER_1 ].config.alarm_en = 1;
    gpio_set_level( self._dPin, 0 );
};