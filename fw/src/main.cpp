#include <Arduino.h>

#include <nvs_flash.h>

#include "dispatcher.h"
#include "worker.h"
#include "frame_buffer.h"
#include "net_source.h"
#include "driver.h"
#include "wifi.h"
#include "commands.h"
#include "transformation.h"

Worker idleWorker;
Dispatcher dispatcher( 30000, GPIO_NUM_27 );
FrameBuffer frameBuffer;
IntDacDriver driver( GPIO_NUM_14 );
NetSource netSource( 4242 );
Transformation transformation;

using TranSet = std::pair< float, float >( Transformation::* )( float, float );
using TranGet = std::pair< float, float >( Transformation::* )();
Packet onTransformation( Packet& p, TranSet set, TranGet get ) {
    std::pair< float, float > ret;
    if ( p.size() == 0 )
        ret = ( transformation.*get )();
    else if ( p.size() == 8 )
        ret = ( transformation.*set )( p.get< float >( 0 ), p.get< float >( 4 ) );
    else
        return makePacket( p.command(), "Invalid payload size" );
    return makePacket( p.command(), ret );
}

Packet onPacket( Packet& p ) {
    Serial.println( "Packet received!" );
    switch ( p.command() ) {
        case COMMAND_FRAME_PUSH:
            Serial.println( "New frame" );
            if ( frameBuffer.push( Frame{ std::move( p ) } ) )
                Serial.println( "Pushed" );
            else
                Serial.println( "Failed" );
            break;
        case COMMAND_TRAN_RESET:
            Serial.println( "Resetting transformation" );
            transformation.reset();
            return makePacket( COMMAND_TRAN_RESET );
        case COMMAND_TRAN_TRANSLATE:
            Serial.println( "Translation" );
            return onTransformation( p, &Transformation::setTranslation, &Transformation::getTranslation );
        case COMMAND_TRAN_ROTATE:
            Serial.println( "Rotation" );
            float ret;
            if ( p.size() == 0 )
                ret = transformation.getRotation();
            else if ( p.size() == 4 )
                ret = transformation.setRotation( p.get< float >( 0 ) );
            else
                return makePacket( COMMAND_TRAN_ROTATE, "Invalid payload size" );
            return makePacket( COMMAND_TRAN_ROTATE, ret );
        case COMMAND_TRAN_SHEAR:
            Serial.println( "Shear" );
            return onTransformation( p, &Transformation::setShear, &Transformation::getShear );
        case COMMAND_TRAN_SKEW:
            Serial.println( "Skew" );
            return onTransformation( p, &Transformation::setSkew, &Transformation::getSkew );
        case COMMAND_TRAN_SCALE:
            Serial.println( "Scale" );
            return onTransformation( p, &Transformation::setScale, &Transformation::getScale );
    }
    return Packet();
}

void onWiFiConnect() {
    Serial.println( "Connected" );
    IPAddress ip = WiFi.localIP();
    Serial.print( "IP Address: " );
    Serial.println( ip );
    netSource.start( onPacket );
}

void onWiFiError( const char* msg ) {
    Serial.print( "Error: " );
    Serial.println( msg );
}

void setup() {
    nvs_flash_init();
    Serial.begin( 115200 );
    Serial.println( "Starting the whole app" );

    transformation.start();
    driver.start();
    frameBuffer.start( []( ImgPoint p ) -> ImgPoint {
        return transformation.transform( p );
    } );
    startWiFi( onWiFiConnect, onWiFiError );

    dispatcher.start( [] {
        driver.show( frameBuffer.nextFromIsr() );
    } );
}

void loop() {
    netSource.accept();
    idleWorker.run();
}