#pragma once

#include <driver/dac.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

extern "C" {
    #include <soc/spi_struct.h>
    #include <soc/spi_reg.h>

    typedef struct {
        spi_device_t *device[ 3 ];
        intr_handle_t intr;
        spi_dev_t *hw;
        spi_transaction_t *cur_trans;
        int cur_cs;
        lldesc_t *dmadesc_tx;
        lldesc_t *dmadesc_rx;
        bool no_gpio_matrix;
        int dma_chan;
        int max_transfer_sz;
    } spi_host_t;

    struct spi_device_t {
        QueueHandle_t trans_queue;
        QueueHandle_t ret_queue;
        spi_device_interface_config_t cfg;
        spi_host_t *host;
    };
}

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

struct Dac8562Driver {
    Dac8562Driver( gpio_num_t din, gpio_num_t sck, gpio_num_t sync,
        gpio_num_t clr, gpio_num_t ldac, gpio_num_t laserPin )
    : _ldac( ldac ),
      _lPin( laserPin )
    {
        gpio_set_direction( clr, GPIO_MODE_OUTPUT );
        gpio_set_level( clr, 1 );

        gpio_set_direction( _ldac, GPIO_MODE_OUTPUT );
        gpio_set_level( _ldac, 1 );

        gpio_set_direction( _lPin, GPIO_MODE_OUTPUT );
        gpio_set_level( _lPin, 0 );

        spi_bus_config_t buscfg;
        buscfg.miso_io_num = -1;
        buscfg.mosi_io_num = din;
        buscfg.sclk_io_num = sck;
        buscfg.quadhd_io_num = -1;
        buscfg.quadwp_io_num = -1;
        buscfg.max_transfer_sz = 0;

        spi_device_interface_config_t devcfg;
        memset( &devcfg, 0, sizeof( devcfg ) );
        devcfg.mode = 1; // Falling edge of SCK
        devcfg.clock_speed_hz = 10'000'000;
        devcfg.spics_io_num = sync;
        devcfg.flags = SPI_DEVICE_HALFDUPLEX;
        devcfg.queue_size = 8;

        esp_err_t ret;

        ret = spi_bus_initialize( HSPI_HOST, &buscfg, 0 /* do not use DMA */ );
        assert( ret == ESP_OK );
        ret = spi_bus_add_device( HSPI_HOST, &devcfg, &_spi );
        assert( ret == ESP_OK );
    }

    void send( uint32_t cmd ) {
        spi_transaction_t t;
        t.flags = SPI_TRANS_USE_TXDATA;
        t.command = 0;
        t.address = 0;
        t.length = 24;
        t.rxlength = 0;
        t.tx_data[ 0 ] = ( cmd >> 16 ) & 0xFF;
        t.tx_data[ 1 ] = ( cmd >> 8  ) & 0xFF;
        t.tx_data[ 2 ] = ( cmd >> 0  ) & 0xFF;
        auto ret = spi_device_transmit( _spi, &t );
        assert( ret == ESP_OK );
    }

    void sendFast( uint32_t data ) {
        //Reset SPI peripheral
        _spi->host->hw->dma_conf.val |= SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST;
        _spi->host->hw->dma_out_link.start=0;
        _spi->host->hw->dma_in_link.start=0;
        _spi->host->hw->dma_conf.val &= ~(SPI_OUT_RST|SPI_IN_RST|SPI_AHBM_RST|SPI_AHBM_FIFO_RST);
        _spi->host->hw->dma_conf.out_data_burst_en=1;
        //Set up QIO/DIO if needed
        _spi-> host->hw->ctrl.val &= ~(SPI_FREAD_DUAL|SPI_FREAD_QUAD|SPI_FREAD_DIO|SPI_FREAD_QIO);
        _spi->host->hw->user.val &= ~(SPI_FWRITE_DUAL|SPI_FWRITE_QUAD|SPI_FWRITE_DIO|SPI_FWRITE_QIO);

        data = ( data & 0xFF )     << 16 |
               ( data & 0xFF00 )   << 0  |
               ( data & 0xFF0000 ) >> 16 ;
        _spi->host->hw->data_buf[ 0 ] = data;
        _spi->host->hw->user.usr_mosi_highpart = 0;

        _spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;
        _spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;

        _spi->host->hw->user.usr_mosi = 1;
        _spi->host->hw->cmd.usr = 1;
        while( _spi->host->hw->cmd.usr );
    }

    void start() {
        // Send as a side effect initializes SPI bus, so sendFast doesn't have to
        send( cmd( 0b111, 0, 1 ) ); // enable internal reference
        send( cmd( 0b110, 0 , 0 ) ); // enable LDAC pin
    }

    void stop() {

    }

    static uint32_t cmd( uint8_t c, uint8_t a, uint16_t d ) {
        return ( static_cast< uint32_t >( c ) & 0x7 ) << 19 |
               ( static_cast< uint32_t >( a ) & 0x7 ) << 16 |
               d;
    }

    void show( ImgPoint p ) {
        uint16_t x = static_cast< int32_t >( p.x ) + 32768;
        uint16_t y = static_cast< int32_t >( p.y ) + 32768;
        sendFast( cmd( 0, 0, x ) );
        sendFast( cmd( 0, 1, y ) );
        gpio_set_level( _ldac, 0 );
        gpio_set_level( _ldac, 1 );
        gpio_set_level( _lPin, 1 );
    }


    static void onFinishedTransfer( spi_transaction_t* t ) {
        uint32_t usr = reinterpret_cast< uint32_t >( t->user );
        auto update = usr >> 16;
        if ( update == 0xFFFF )
            return;
        auto pin = static_cast< gpio_num_t >( update );
        gpio_set_level( pin, 0 );
        gpio_set_level( pin, 1 );

        auto laserPin = static_cast< gpio_num_t >( usr & 0xFF );
        auto level = ( usr >> 8 ) & 0xFF;
        gpio_set_level( laserPin, level );
    }

    gpio_num_t _ldac, _lPin;
    spi_device_handle_t _spi;
};