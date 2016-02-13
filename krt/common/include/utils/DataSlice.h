#pragma once

// Original copyright message follows.

/*****************************************************************************
*
*  PROJECT:     MTA:Eir
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        eirrepo/sdk/MemoryRaw.h
*  PURPOSE:     Base memory management definitions for to-the-metal things
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

namespace krt
{
template <typename numberType>
class sliceOfData
{
public:
    inline sliceOfData( void )
    {
        this->startOffset = 0;
        this->endOffset = -1;
    }

    inline sliceOfData( numberType startOffset, numberType dataSize )
    {
        this->startOffset = startOffset;
        this->endOffset = startOffset + ( dataSize - 1 );
    }

    enum eIntersectionResult
    {
        INTERSECT_EQUAL,
        INTERSECT_INSIDE,
        INTERSECT_BORDER_START,
        INTERSECT_BORDER_END,
        INTERSECT_ENCLOSING,
        INTERSECT_FLOATING_START,
        INTERSECT_FLOATING_END,
        INTERSECT_UNKNOWN   // if something went horribly wrong (like NaNs).
    };

    // Static methods for easier result management.
    static bool isBorderIntersect( eIntersectionResult result )
    {
        return ( result == INTERSECT_BORDER_START || result == INTERSECT_BORDER_END );
    }

    static bool isFloatingIntersect( eIntersectionResult result )
    {
        return ( result == INTERSECT_FLOATING_START || result == INTERSECT_FLOATING_END );
    }

    inline numberType GetSliceSize( void ) const
    {
        return ( this->endOffset - this->startOffset ) + 1;
    }

    inline void SetSlicePosition( numberType val )
    {
        const numberType sliceSize = GetSliceSize();

        this->startOffset = val;
        this->endOffset = val + ( sliceSize - 1 );
    }

    inline void OffsetSliceBy( numberType val )
    {
        SetSlicePosition( this->startOffset + val );
    }

    inline void SetSliceStartPoint( numberType val )
    {
        this->startOffset = val;
    }

    inline void SetSliceEndPoint( numberType val )
    {
        this->endOffset = val;
    }

    inline numberType GetSliceStartPoint( void ) const
    {
        return startOffset;
    }

    inline numberType GetSliceEndPoint( void ) const
    {
        return endOffset;
    }

    inline eIntersectionResult intersectWith( const sliceOfData& right ) const
    {
        // Make sure the slice has a valid size.
        if ( this->endOffset >= this->startOffset &&
             right.endOffset >= right.startOffset )
        {
            // Get generic stuff.
            numberType sliceStartA = startOffset, sliceEndA = endOffset;
            numberType sliceStartB = right.startOffset, sliceEndB = right.endOffset;

            // slice A -> this
            // slice B -> right

            // Handle all cases.
            // We only implement the logic with comparisons only, as it is the most transparent for all number types.
            if ( sliceStartA == sliceStartB && sliceEndA == sliceEndB )
            {
                // Slice A is equal to Slice B
                return INTERSECT_EQUAL;
            }

            if ( sliceStartB >= sliceStartA && sliceEndB <= sliceEndA )
            {
                // Slice A is enclosing Slice B
                return INTERSECT_ENCLOSING;
            }

            if ( sliceStartB <= sliceStartA && sliceEndB >= sliceEndA )
            {
                // Slice A is inside Slice B
                return INTERSECT_INSIDE;
            }

            if ( sliceStartB < sliceStartA && ( sliceEndB >= sliceStartA && sliceEndB <= sliceEndA ) )
            {
                // Slice A is being intersected at the starting point.
                return INTERSECT_BORDER_START;
            }

            if ( sliceEndB > sliceEndA && ( sliceStartB >= sliceStartA && sliceStartB <= sliceEndA ) )
            {
                // Slice A is being intersected at the ending point.
                return INTERSECT_BORDER_END;
            }

            if ( sliceStartB < sliceStartA && sliceEndB < sliceStartA )
            {
                // Slice A is after Slice B
                return INTERSECT_FLOATING_END;
            }

            if ( sliceStartB > sliceEndA && sliceEndB > sliceEndA )
            {
                // Slice A is before Slice B
                return INTERSECT_FLOATING_START;
            }
        }

        return INTERSECT_UNKNOWN;
    }

private:
    numberType startOffset;
    numberType endOffset;
};
}