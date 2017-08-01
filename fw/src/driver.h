#pragma once

#include <driver/dac.h>
#include <driver/gpio.h>

struct IntDacDriver {
    IntDacDriver( gpio_num_t laserPin )
        : _lPin( laserPin )
    { }

    ~IntDacDriver() {
        stop();
    }

    void start() {
        dac_output_enable( DAC_CHANNEL_1 ); // GPIO 25
        dac_output_enable( DAC_CHANNEL_2 ); // GPIO 26
        gpio_set_direction( _lPin, GPIO_MODE_OUTPUT );
        gpio_set_level( _lPin, 0 );
    }

    void stop() {
        gpio_set_level( _lPin, 0 );
    }

    void show( ImgPoint p ) {
        dac_output_voltage( DAC_CHANNEL_1, 128 + ( p.x / 255 ) );
        dac_output_voltage( DAC_CHANNEL_2, 128 + ( p.y / 255 ) );
        gpio_set_level( _lPin, p.l > 0 );
    }

    gpio_num_t _lPin;
};