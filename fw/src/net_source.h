#pragma once

#include <functional>

#include <Arduino.h>
#include <WiFi.h>

#include "packet.h"

struct NetSource {
    NetSource( int port, int maxConnections = 4 )
        : _socketServer( port, maxConnections ),
          _queue( xQueueCreate( maxConnections, sizeof( WiFiClient* ) ) )
    {
        char name[ 5 ] = { 'N', 'e', 't', '0', '\0' };
        for ( int i = 0; i != maxConnections; i++ ) {
            name[ 3 ]++;
            xTaskCreate( handleClient, name, 4096, this, 5, nullptr );
        }
    }

    template < typename F >
    void start( F packetHandler ) {
        _packetHandler = packetHandler;
        _socketServer.begin();
    }

    void accept() {
        auto client = std::make_unique< WiFiClient >( _socketServer.available() );
        if ( *client ) {
            WiFiClient* clientPtr = client.get();
            if ( xQueueSend( _queue, &clientPtr, 0 ) == pdTRUE )
                client.release();
        }
    }

    static void handleClient( void *arg ) {
        static const int BUFFER_SIZE = 4096;
        std::unique_ptr< uint8_t[] > buffer{ new uint8_t[ BUFFER_SIZE ] };
        auto& self = *reinterpret_cast< NetSource* >( arg );
        while ( true ) {
            WiFiClient* clientPtr;
            if ( xQueueReceive( self._queue, &clientPtr, portMAX_DELAY ) == pdFALSE )
                continue;
            Serial.println( "New client" );
            std::unique_ptr< WiFiClient > client( clientPtr );
            Packet packet;
            unsigned long nextKeepAlive = millis() + 1000;
            unsigned long packetTimestamp;
            bool first = true;
            while ( client->connected() ) {
                while ( client->available() ) {
                    if ( first )
                        packetTimestamp = millis();
                    first = false;
                    int size = std::min( client->available(), BUFFER_SIZE );
                    Serial.println( size );
                    client->read( buffer.get(), size );
                    for ( int i = 0; i != size; i++ ) {
                        if ( packet.read( buffer[ i ] ) ) {
                            Serial.print( "A: " );
                            Serial.println( millis() - packetTimestamp );
                            auto response = self._packetHandler( packet );
                            Serial.print( "B: " );
                            Serial.println( millis() - packetTimestamp );
                            client->write( response.data(), response.size() );
                            Serial.print( "C: " );
                            Serial.println( millis() - packetTimestamp );
                            packet.reset();
                            first = true;
                        }
                    }
                }
                if ( millis() > nextKeepAlive ) {
                    nextKeepAlive = millis() + 1000;
                    static const char keepAlive[] = { 128, 0, 0, 30 };
                    client->write( keepAlive, 4 );
                }
                delay( 1 );
            }
            client->stop();
            Serial.println( "Client disconnected" );
        }
    }

    std::function< Packet( Packet& ) > _packetHandler;
    WiFiServer _socketServer;
    QueueHandle_t _queue;
};