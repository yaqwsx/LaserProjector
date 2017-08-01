#include <Preferences.h>

#include <utility>
#include <string>
#include <map>
#include <iostream>
#include <limits>

#include <include/atoms/numeric/naive_fixed_printer.h>
#include <include/atoms/numeric/matrix.h>

#include "packet.h"
#include "img_point.h"

struct Transformation {

    using Float = atoms::Fixed< 32, 32 >;
    using TMatrix = atoms::Matrix< Float, 3, 3 >;
    using Vector = atoms::Matrix< Float, 1, 3 >;

    void start() {
        _pref.begin( "LP" );
        updateMatrix();
    }

    ImgPoint transform( ImgPoint x ) {
        Vector y = _matrix * Vector( { { Float( x.x ) }, { Float( x.y ) }, { Float( 1 ) } } );
        int a = ( y[ 0 ][ 0 ] / y[ 2 ][ 0 ] ).to_signed();
        const int cropTop = std::numeric_limits< int16_t >::max() * 0.95;
        const int cropBottom = std::numeric_limits< int16_t >::min() * 0.95;
        a = std::min( cropTop, a );
        a = std::max( cropBottom, a );
        int b = ( y[ 1 ][ 0 ] / y[ 2 ][ 0 ] ).to_signed();
        b = std::min( cropTop, b );
        b = std::max( cropBottom, b );
        x.x = a;
        x.y = b;
        return x;
    }

    TMatrix _getUnitMatrix() {
        TMatrix m( Float{ 0 } );
        for ( int i = 0; i != 3; i++ )
            m[ i ][ i ] = Float{ 1 };
        return m;
    }

    void updateMatrix() {
        std::cout.flush();
        TMatrix tran = _getUnitMatrix();
        tran[ 0 ][ 2 ] = Float( load( "transX" ) );
        tran[ 1 ][ 2 ] = Float( load( "transY") );

        float r = load( "rot" );
        TMatrix rot( 0 );
        rot[ 0 ][ 0 ] = cos( r );
        rot[ 0 ][ 1 ] = -sin( r );
        rot[ 1 ][ 0 ] = sin( r );
        rot[ 1 ][ 1 ] = cos( r );
        rot[ 2 ][ 2 ] = 1;

        TMatrix shear = _getUnitMatrix();
        shear[ 0 ][ 1 ] = Float( load( "shearX" ) );
        shear[ 1 ][ 0 ] = Float( load( "shearY" ) );

        TMatrix trap = _getUnitMatrix();
        shear[ 2 ][ 0 ] = Float( load( "trapX" ) );
        shear[ 2 ][ 1 ] = Float( load( "trapY" ) );

        TMatrix trapMin = _getUnitMatrix();
        trapMin[ 0 ][ 0 ] = Float( 2.0 / 4096 );
        trapMin[ 1 ][ 1 ] = Float( 2.0 / 4096 );

        TMatrix trapMax = _getUnitMatrix();
        trapMax[ 0 ][ 0 ] = Float( 4096 / 2.0 );
        trapMax[ 1 ][ 1 ] = Float( 4096 / 2.0 );

        _matrix = tran * trap * shear * rot;
        std::cout << _matrix << "\n";
        std::cout << trapMin << "\n";
        std::cout << trapMax << "\n";
        std::cout << ( trapMin * trapMax );
        std::cout.flush();
    }

    void store( const char* key, float v ) {
        // _pref.putULong( key, *reinterpret_cast< uint32_t* >( &v ) );
        _params[ key ] = v;
    }

    float load( const char* key ) {
        // uint32_t v = _pref.getULong( key );
        // return *reinterpret_cast< float* >( &v );
        return _params[ key ];
    }

    void reset() {
        store( "transX", 0 );
        store( "transY", 0 );
        store( "rot", 0 );
        store( "shearX", 0 );
        store( "shearY", 0 );
        store( "trapX", 0 );
        store( "trapY", 0 );
        updateMatrix();
    }

    std::pair< float, float > getTranslation() {
        return { load( "transX" ), load( "transY") };
    }

    std::pair< float, float > setTranslation( float x, float y ) {
        auto prev = getTranslation();
        store( "transX", x );
        store( "transY", y );
        updateMatrix();
        return prev;
    }

    float getRotation() {
        return load( "rot" );
    }

    float setRotation( float x ) {
        auto prev = getRotation();
        store( "rot", x );
        updateMatrix();
        return prev;
    }

    std::pair< float, float > getShear() {
        return { load( "shearX" ), load( "shearY") };
    }

    std::pair< float, float > setShear( float x, float y ) {
        auto prev = getShear();
        store( "shearX", x );
        store( "shearY", y );
        updateMatrix();
        return prev;
    }

    std::pair< float, float > getTrapez() {
        return { load( "trapX" ), load( "trapY") };
    }

    std::pair< float, float > setTrapez( float x, float y ) {
        auto prev = getTrapez();
        store( "trapX", x );
        store( "trapY", y );
        updateMatrix();
        return prev;
    }

    Preferences _pref;
    TMatrix _matrix;
    std::map< std::string, float > _params;
};