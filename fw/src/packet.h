#pragma once

#include <cstdint>
#include <vector>

template < typename Char >
struct PacketImpl {
    using HeaderType = uint8_t;
    using CommandType = uint8_t;
    using SizeType = uint16_t;

    static const constexpr int DataOffset =
        sizeof( HeaderType ) + sizeof( SizeType ) + sizeof( CommandType );

    PacketImpl()
        : _data( 4 )
    {
        reset();
    }

    auto begin() {
        return _data.begin();
    }

    auto end() {
        return _data.end();
    }

    void reset() {
        _read = 0;
        _data.resize( DataOffset );
        _as< HeaderType >( 0 ) = 0x80;
        _as< uint16_t >( sizeof( HeaderType ) ) = 0;
        setCommand( 0 );
    }

    void setCommand( uint8_t command ) {
        _as< uint8_t >( 3 ) = command;
    }

    uint8_t command() const {
        return _as< uint8_t >( sizeof( HeaderType ) + sizeof( SizeType ) );
    }

    uint16_t size() const {
        return _as< uint16_t >( sizeof( HeaderType ) );
    }

    int rawSize() const {
        return _data.size();
    }

    Char* data() {
        return _data.data();
    }

    template < typename T >
    T& _as( int offset ) {
        return *reinterpret_cast< T* >( _data.data() + offset );
    }

    template < typename T >
    const T& _as( int offset ) const {
        return *reinterpret_cast< const T* >( _data.data() + offset );
    }

    template < typename T >
    T get( int offset ) const {
        return _as< T >( offset + DataOffset );
    }

    template < typename T >
    void push( const T& t ) {
        int size = _data.size();
        _data.resize( size + sizeof( T ) );
        _as< T >( size ) = t;
        _as< uint16_t >( sizeof( HeaderType ) ) += sizeof( T );
    }

    template < typename T >
    void push_n( const T* t, int n ) {
        int size = _data.size();
        _data.resize( size + n * sizeof( T ) );
        for ( int i = 0; i != n; i++ ) {
            _as< T >( size + i * sizeof( T ) ) = t[ i ];
        }
        _as< uint16_t >( sizeof( HeaderType ) ) += n * sizeof( T );
    }

    void push( const char* s ) {
        push_n( s, strlen( s ) + 1 );
    }

    bool read( Char c ) {
        if ( _read == 0 ) {
            if ( c == 0x80 ) {
                _data[ 0 ] = 0x80;
                _read++;
            }
            return false;
        }

        if ( _read < sizeof( HeaderType ) + sizeof( SizeType ) ) {
            _data[ _read ] = c;
            _read++;
            return false;
        }

        if ( _read < DataOffset ) {
            _data [ _read ] = c;
            _read++;
            if ( _read < DataOffset )
                return false;
            return _data.size() == DataOffset + size();
        }
        _data.reserve( size() + DataOffset );
        _data.push_back( c );
        return _data.size() == DataOffset + size();
    }

    std::vector< Char > _data;
    int _read;
};

using Packet = PacketImpl< char >;

void _pushInPacket( Packet& p ) {}

template < typename T, typename... Args >
void _pushInPacket( Packet& p, T t, Args... args ) {
    p.push< T >( t );
    _pushInPacket( p, args... );
}

template < typename... Args >
Packet makePacket( uint8_t command, Args... args ) {
    Packet p;
    p.setCommand( command );
    _pushInPacket( p, args... );
    return p;
}
