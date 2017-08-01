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

    using Float = atoms::Fixed< 20, 12 >;
    using TMatrix = atoms::Matrix< Float, 3, 3 >;
    using Vector = atoms::Matrix< Float, 1, 3 >;

    void start() {
        _pref.begin( "LP" );
        reset(); // Until NVS works
        updateMatrix();
    }

    ImgPoint transform( ImgPoint x ) {
        Vector y = _matrix * Vector( { { Float( x.x ) }, { Float( x.y ) }, { Float( 1 ) } } );
        int64_t a = ( y[ 0 ][ 0 ] / y[ 2 ][ 0 ] ).to_signed();
        int64_t b = ( y[ 1 ][ 0 ] / y[ 2 ][ 0 ] ).to_signed();
        const int64_t cropTop = std::numeric_limits< int16_t >::max() * 0.95;
        const int64_t cropBottom = -cropTop;
        // Do skew separately, as it has problems with precision in the matrix form
        a = a * ( 65535 * cropTop + _skewX * b ) / ( cropTop * 65535 );
        b = b * ( 65535 * cropTop + _skewY * a ) / ( cropTop * 65535 );
        a = std::min( cropTop, a );
        a = std::max( cropBottom, a );
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
        for ( const auto& x : _params )
            std::cout << x.first << " -> " << x.second << "\n";
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

        TMatrix skew = _getUnitMatrix();
        skew[ 2 ][ 0 ] = Float( load( "skewX" ) );
        skew[ 2 ][ 1 ] = Float( load( "skewY" ) );
        // Do skew separately, as it has problems with precision in the matrix form
        _skewX = load( "skewX" );
        _skewY = load( "skewY" );

        TMatrix scale = _getUnitMatrix();
        scale[ 0 ][ 0 ] = Float( load( "scaleX" ) );
        scale[ 1 ][ 1 ] = Float( load( "scaleY" ) );

        _matrix = tran * rot * shear * /* skew */ scale;
        std::cout << "Transformation:\n" << scale << "\n";
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
        store( "skewX", 0 );
        store( "skewY", 0 );
        store( "scaleX", 1 );
        store( "scaleY", 1 );
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

    std::pair< float, float > getSkew() {
        return { load( "skewX" ), load( "skewY") };
    }

    std::pair< float, float > setSkew( float x, float y ) {
        auto prev = getSkew();
        store( "skewX", x );
        store( "skewY", y );
        updateMatrix();
        return prev;
    }

    std::pair< float, float > getScale() {
        return { load( "scaleX" ), load( "scaleY") };
    }

    std::pair< float, float > setScale( float x, float y ) {
        auto prev = getScale();
        store( "scaleX", x );
        store( "scaleY", y );
        updateMatrix();
        return prev;
    }

    Preferences _pref;
    TMatrix _matrix;
    int _skewX, _skewY;
    std::map< std::string, float > _params;
};