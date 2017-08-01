#pragma once

#include <WiFi.h>

#include "credentials.h"

template < typename F1, typename F2 >
void startWiFi( F1 onConnect, F2 onError ) {
    while ( true ) {
        WiFi.mode( WIFI_MODE_STA );
        WiFi.begin( ssid, password );
        bool error = false;
        while ( !error ) {
            auto status = WiFi.status();
            switch( status ) {
                case WL_CONNECTED:
                    onConnect();
                    return;
                case WL_IDLE_STATUS:
                case WL_DISCONNECTED:
                    delay( 100 );
                    break;
                case WL_NO_SHIELD:
                    onError( "No shield error" );
                    error = true;
                    break;
                case WL_NO_SSID_AVAIL:
                    onError( "No SSID" );
                    error = true;
                    break;
                case WL_CONNECT_FAILED:
                    onError( "Connect failed" );
                    error = true;
                    break;
                case WL_CONNECTION_LOST:
                    onError( "Connection lost" );
                    error = true;
                    break;
            }
        }
    }
}