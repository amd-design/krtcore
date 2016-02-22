#pragma once

// Basic QuadTree implementation for world sectoring.
// It works recursively until a predefined depth.

#include "WorldMath.h"

#include <type_traits>

namespace krt
{

template <size_t depth, size_t boundDimm, typename DataType>
struct QuadTree
{
    inline QuadTree( void )
    {

    }

    inline ~QuadTree( void )
    {

    }

    template <typename numberType, size_t minX, size_t minY, size_t maxX, size_t maxY>
    MATH_INLINE static bool IsPointInBounds( numberType x, numberType y )
    {
        return ( x >= minX && x >= minY && x < maxX && y < maxY );
    }

    template <typename numberType, size_t minX, size_t minY, size_t maxX, size_t maxY>
    MATH_INLINE static bool IsBoundsInBounds( numberType box_minX, numberType box_minY, numberType box_maxX, numberType box_maxY )
    {
        return ( false );   // TODO.
    }

    template <size_t depth, size_t boundDimm, typename DataType, size_t xOff, size_t yOff>
    struct Node
    {
        inline Node( void )
        {

        }

        inline ~Node( void )
        {

        }

        static size_t constexpr boundDimm = boundDimm;

    private:
        static constexpr size_t subdimm_bound = ( boundDimm / 2 );

        Node <depth - 1, subdimm_bound, DataType, xOff, yOff> topLeft;
        Node <depth - 1, subdimm_bound, DataType, xOff + subdimm_bound, yOff> topRight;
        Node <depth - 1, subdimm_bound, DataType, xOff, yOff + subdimm_bound> bottomLeft;
        Node <depth - 1, subdimm_bound, DataType, xOff + subdimm_bound, yOff + subdimm_bound> bottomRight;

    public:
        template <typename numberType, typename callbackType>
        MATH_INLINE void VisitByPoint( numberType x, numberType y, callbackType& cb )
        {
            // Each node takes care of bound checking itself.
            if ( IsPointInBounds <numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm> ( x, y ) )
            {
                topLeft.VisitByPoint( x, y, cb );
                topRight.VisitByPoint( x, y, cb );
                bottomLeft.VisitByPoint( x, y, cb );
                bottomRight.VisitByPoint( x, y, cb );
            }
        }
    };

    template <size_t boundDimm, typename DataType, size_t xOff, size_t yOff>
    struct Node <0, boundDimm, DataType, xOff, yOff>
    {
        inline Node( void )
        {

        }

        inline ~Node( void )
        {
            
        }

        static size_t constexpr boundDimm = boundDimm;

    private:
        DataType data;

    public:
        template <typename numberType, typename callbackType>
        MATH_INLINE void VisitByPoint( numberType x, numberType y, callbackType& cb )
        {
            // Each node takes care of bound checking itself.
            if ( IsPointInBounds <numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm> ( x, y ) )
            {
                // Call the thing.
                cb( this->data );
            }
        }
    };

    Node <depth, boundDimm, DataType, 0, 0> root;
};

}