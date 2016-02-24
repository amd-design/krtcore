#include "StdInc.h"
#include "WorldMath.h"

#include <utils/DataSlice.h>

namespace krt
{
namespace math
{

inline float ivec( const rw::V3d& v, int idx )
{
    if ( idx == 1 )
    {
        return v.x;
    }
    else if ( idx == 2 )
    {
        return v.y;
    }
    else if ( idx == 3 )
    {
        return v.z;
    }

    return 0.0f;
}

// Cats meow beautifully into the night
typedef double p_resolve_t;

// Class to represent a polynome of max. first grade.
struct Simple_Polynome
{
    p_resolve_t raise;
    p_resolve_t const_off;

    MATH_INLINE p_resolve_t Calculate( p_resolve_t x ) const
    {
        return ( raise * x + const_off );
    }

    MATH_INLINE bool IsConst( void ) const
    {
        return ( this->raise == 0 );
    }
};

enum class ePolynomePlaneDependsType
{
    DEPENDS_ON_A,
    DEPENDS_ON_B,
    DEPENDS_ON_S,
    DEPENDS_ON_T
};

enum class ePolynomeCoefficient
{
    COEFF_A,
    COEFF_B,
    COEFF_S,
    COEFF_T
};

// Helper math.
MATH_INLINE p_resolve_t block_trans( const rw::V3d& t, const rw::V3d& b, int idxPrim, int idxSec )
{
    return ivec( t, idxPrim ) / ivec( b, idxPrim ) - ivec( t, idxSec ) / ivec( b, idxSec );
}

// The most general solution, kinda. Could also say most complicated.
inline bool plane_solution_fetcht_type_a(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& coeffSrcType, ePolynomeCoefficient& coeffDependsType
)
{
    // Check this shit beforehand.
    assert( ivec( x, 1 ) != 0 && ivec( x, 2 ) != 0 && ivec( x, 3 ) != 0 );

    p_resolve_t mod_point_yx12 = block_trans( y, x, 1, 2 );
    p_resolve_t mod_point_yx32 = block_trans( y, x, 3, 2 );

    p_resolve_t mod_point_yx12_yx32_diff = ( mod_point_yx12 - mod_point_yx32 ); // MUST NOT BE ZERO.

    p_resolve_t mod_point_ux12 = block_trans( u, x, 1, 2 ); // MUST NOT BE ZERO
    p_resolve_t mod_point_ux32 = block_trans( u, x, 3, 2 ); // MUST NOT BE ZERO

    if ( mod_point_yx12_yx32_diff != 0 && mod_point_ux12 != 0 && mod_point_ux32 != 0 )
    {
        polyOut.raise = ( block_trans( z, x, 1, 2 ) / mod_point_ux12 - block_trans( z, x, 3, 2 ) / mod_point_ux32 ) / mod_point_yx12_yx32_diff;
        polyOut.const_off = 
            (
                (
                    ( block_trans( o, x, 1, 2 ) - block_trans( p, x, 1, 2 ) ) / mod_point_ux12
                    -
                    ( block_trans( o, x, 3, 2 ) - block_trans( p, x, 3, 2 ) ) / mod_point_ux32
                )
            )
            / mod_point_yx12_yx32_diff;

        coeffSrcType = ePolynomeCoefficient::COEFF_T;
        coeffDependsType = ePolynomeCoefficient::COEFF_B;

        return true;
    }

    p_resolve_t mod_point_yx13 = block_trans( y, x, 1, 3 );
    
    p_resolve_t mod_point_yx12_yx13_diff = ( mod_point_yx12 - mod_point_yx13 ); // MUST NOT BE ZERO

    p_resolve_t mod_point_ux13 = block_trans( u, x, 1, 3 ); // MUST NOT BE ZERO

    if ( mod_point_yx12_yx13_diff != 0 && mod_point_ux12 != 0 && mod_point_ux13 != 0 )
    {
        polyOut.raise = ( block_trans( z, x, 1, 2 ) / mod_point_ux12 - block_trans( z, x, 1, 3 ) / mod_point_ux13 ) / mod_point_yx12_yx13_diff;
        polyOut.const_off =
            (
                (
                    ( block_trans( o, x, 1, 2 ) - block_trans( p, x, 1, 2 ) ) / mod_point_ux12
                    -
                    ( block_trans( o, x, 1, 3 ) - block_trans( p, x, 1, 3 ) ) / mod_point_ux13
                )
            )
            / mod_point_yx12_yx13_diff;

        coeffSrcType = ePolynomeCoefficient::COEFF_T;
        coeffDependsType = ePolynomeCoefficient::COEFF_B;

        return true;
    }

    p_resolve_t mod_point_yx32_yx13_diff = ( mod_point_yx32 - mod_point_yx13 );

    if ( mod_point_yx32_yx13_diff != 0 && mod_point_ux32 != 0 && mod_point_ux13 != 0 )
    {
        polyOut.raise = ( block_trans( z, x, 3, 2 ) / mod_point_ux32 - block_trans( z, x, 1, 3 ) / mod_point_ux13 ) / mod_point_yx32_yx13_diff;
        polyOut.const_off = 
            (
                (
                    ( block_trans( o, x, 3, 2 ) - block_trans( p, x, 3, 2 ) ) / mod_point_ux32
                    -
                    ( block_trans( o, x, 1, 3 ) - block_trans( p, x, 1, 3 ) ) / mod_point_ux13
                )
            )
            / mod_point_yx32_yx13_diff;

        coeffSrcType = ePolynomeCoefficient::COEFF_T;
        coeffDependsType = ePolynomeCoefficient::COEFF_B;

        return true;
    }

    // No solution :(
    // At least for this type of equation.
    return false;
}

// A sort of less complicated solution, but still coolios.
inline bool plane_solution_fetcht_type_b(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& coeffSrcType, ePolynomeCoefficient& coeffDependsType,
    int datIndex
)
{
    assert( ivec( x, datIndex ) == 0 );

    // Figure out the valid permutation to use, meow.
    // Unlike the hyper complex solution type_a, we only have one possible equation, meow.
    int prim_divide = ( ( datIndex == 1 ) ? 2 : 1 );
    int sec_divide = ( ( datIndex == 3 ) ? 2 : 3 );

    // But the question is: what is the friggin solution equation actually?
    // Here is the answer! Lets search.

    if ( ivec( x, prim_divide ) != 0 && ivec( x, sec_divide ) != 0 )
    {
        if ( ivec( u, datIndex ) != 0 )
        {
            // We got a solution, probably.
            p_resolve_t mod_point_ux = block_trans( u, x, prim_divide, sec_divide );

            if ( mod_point_ux != 0 )
            {
                p_resolve_t major_divisor = ( ivec( y, datIndex ) / ivec( u, datIndex ) - block_trans( y, x, prim_divide, sec_divide ) );

                if ( major_divisor != 0 )
                {
                    // Alright, we should have a solution here.
                    polyOut.raise = ( ivec( z, datIndex ) / ivec( u, datIndex ) - block_trans( z, x, prim_divide, sec_divide ) ) / major_divisor;
                    polyOut.const_off =
                        (
                            (
                                ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( u, datIndex )
                                -
                                (
                                    ( ivec( o, prim_divide ) - ivec( p, prim_divide ) ) / ivec( x, prim_divide )
                                    -
                                    ( ivec( o, sec_divide ) - ivec( p, sec_divide ) ) / ivec( x, sec_divide )
                                )
                                /
                                mod_point_ux
                            )
                        ) / major_divisor;

                    coeffSrcType = ePolynomeCoefficient::COEFF_T;
                    coeffDependsType = ePolynomeCoefficient::COEFF_B;

                    return true;
                }
            }

            // Welp, we have no solution yet.
            // We kinda should search further.

            if ( ivec( u, prim_divide ) == 0 || ivec( u, sec_divide ) == 0 )
            {
                p_resolve_t modulator;
                bool hasModulator = false;

                if ( ivec( u, prim_divide ) == 0 && ivec( x, sec_divide ) != 0 )
                {
                    modulator = -ivec( u, sec_divide ) / ivec( x, sec_divide );

                    hasModulator = true;
                }
                else if ( ivec( u, sec_divide ) == 0 && ivec( x, prim_divide ) != 0 )
                {
                    modulator = ivec( u, prim_divide ) / ivec( x, prim_divide );

                    hasModulator = true;
                }

                if ( hasModulator && modulator != 0 )
                {
                    // Maybe we have a solution here? We cannot say yet.
                    p_resolve_t major_divisor = ( ivec( y, datIndex ) / ivec( u, datIndex ) - block_trans( y, x, prim_divide, sec_divide ) / modulator );

                    if ( major_divisor != 0 )
                    {
                        // We got a winner!
                        polyOut.raise = ( ivec( z, datIndex ) / ivec( u, datIndex ) - block_trans( z, x, prim_divide, sec_divide ) / modulator ) / major_divisor;
                        polyOut.const_off =
                            (
                                (
                                    ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( u, datIndex )
                                    -
                                    (
                                        ( ivec( o, prim_divide ) - ivec( p, prim_divide ) ) / ivec( x, prim_divide )
                                        -
                                        ( ivec( o, sec_divide ) - ivec( p, sec_divide ) ) / ivec( x, sec_divide )
                                    )
                                ) / modulator
                            ) / major_divisor;

                        coeffSrcType = ePolynomeCoefficient::COEFF_T;
                        coeffDependsType = ePolynomeCoefficient::COEFF_B;

                        return true;
                    }
                }
            }

            // There are still more possibilities, m8!

            if ( ivec( u, prim_divide ) == 0 && ivec( u, sec_divide ) == 0 )
            {
                // We could have an easy solution here.
                // But not always!
                p_resolve_t divisor = block_trans( y, x, prim_divide, sec_divide );

                if ( divisor != 0 )
                {
                    // Easy fish.
                    polyOut.raise = block_trans( z, x, prim_divide, sec_divide ) / divisor;
                    polyOut.const_off =
                        (
                            (
                                ( ivec( o, prim_divide ) - ivec( p, prim_divide ) ) / ivec( x, prim_divide )
                                -
                                ( ivec( o, sec_divide ) - ivec( p, sec_divide ) ) / ivec( x, sec_divide )
                            )
                        ) / divisor;

                    coeffSrcType = ePolynomeCoefficient::COEFF_T;
                    coeffDependsType = ePolynomeCoefficient::COEFF_B;

                    return true;
                }
            }
        }
    }

    // And even more...

    if ( ivec( u, datIndex ) == 0 )
    {
        if ( ivec( y, datIndex ) != 0 )
        {
            // Pretty cool little solution right here.
            polyOut.raise = ivec( z, datIndex ) / ivec( y, datIndex );
            polyOut.const_off = ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( y, datIndex );

            coeffSrcType = ePolynomeCoefficient::COEFF_T;
            coeffDependsType = ePolynomeCoefficient::COEFF_B;

            return true;
        }
        else
        {
            if ( ivec( z, datIndex ) != 0 ) // Otherwise 2D plane equation stuff. Maybe in the future.
            {
                rw::V3d op_diff = rw::sub( o, p );

                // b is a constant.
                p_resolve_t b = ( -ivec( op_diff, datIndex ) / ivec( z, datIndex ) );

                if ( ivec( x, prim_divide ) == 0 )
                {
                    if ( ivec( y, prim_divide ) != 0 )  // otherwise the other plane makes no sense.
                    {
                        polyOut.raise = ivec( u, prim_divide ) / ivec( y, prim_divide );
                        polyOut.const_off = ( b * ivec( z, prim_divide ) + ivec( op_diff, prim_divide ) ) / ivec( y, prim_divide );

                        coeffSrcType = ePolynomeCoefficient::COEFF_T;
                        coeffDependsType = ePolynomeCoefficient::COEFF_A;

                        return true;
                    }
                }
                else if ( ivec( x, sec_divide ) == 0 )
                {
                    if ( ivec( y, sec_divide ) != 0 )  // otherwise the other plane makes no sense.
                    {
                        polyOut.raise = ivec( u, sec_divide ) / ivec( y, sec_divide );
                        polyOut.const_off = ( b * ivec( z, sec_divide ) + ivec( op_diff, sec_divide ) ) / ivec( y, sec_divide );

                        coeffSrcType = ePolynomeCoefficient::COEFF_T;
                        coeffDependsType = ePolynomeCoefficient::COEFF_A;

                        return true;
                    }
                }
                else
                {
                    // Special solution.
                    p_resolve_t divisor = block_trans( y, x, prim_divide, sec_divide );

                    if ( divisor != 0 )
                    {
                        // We can get a special kind of equation for t.
                        polyOut.raise = block_trans( u, x, prim_divide, sec_divide ) / divisor;
                        polyOut.const_off = ( b * block_trans( z, x, prim_divide, sec_divide ) + block_trans( op_diff, x, prim_divide, sec_divide ) ) / divisor;

                        coeffSrcType = ePolynomeCoefficient::COEFF_T;
                        coeffDependsType = ePolynomeCoefficient::COEFF_A;

                        return true;
                    }
                }
            }
        }
    }

    // No result.
    return false;
}

// The cutest solution yet.
inline bool plane_solution_fetcht_type_c(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& coeffSrcType, ePolynomeCoefficient& coeffDependsType,
    int prim_idx, int sec_idx
)
{
    assert( ivec( x, prim_idx ) == 0 && ivec( x, sec_idx ) == 0 );

    if ( ivec( u, prim_idx ) != 0 && ivec( u, sec_idx ) != 0 )
    {
        // It ain't that hard even, meow.
        p_resolve_t divisor = block_trans( y, u, prim_idx, sec_idx );

        if ( divisor )
        {
            polyOut.raise = block_trans( z, u, prim_idx, sec_idx ) / divisor;
            polyOut.const_off = 
                (
                    (
                        ( ivec( o, prim_idx ) - ivec( p, prim_idx ) ) / ivec( u, prim_idx )
                        -
                        ( ivec( o, sec_idx ) - ivec( p, sec_idx ) ) / ivec( u, sec_idx )
                    )
                ) / divisor;

            coeffSrcType = ePolynomeCoefficient::COEFF_T;
            coeffDependsType = ePolynomeCoefficient::COEFF_B;

            return true;
        }
    }

    return false;
}

// As well as for the other polynome calculator, we can have corner case polynomes here.
MATH_INLINE bool plane_solution_fetcht_type_d(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& coeffSrcType, ePolynomeCoefficient& coeffDependsType,
    int datIndex
)
{
    int prim_divide = ( ( datIndex == 1 ) ? 2 : 1 );
    int sec_divide = ( ( datIndex == 3 ) ? 2 : 3 );

    if ( ivec( y, datIndex ) != 0 && ivec( x, datIndex ) == 0 && ivec( y, prim_divide ) == 0 && ivec( y, sec_divide ) == 0 &&
         ivec( z, datIndex ) == 0 )
    {
        // Special cute corner case solution, meow.
        polyOut.raise = ivec( u, datIndex ) / ivec( y, datIndex );
        polyOut.const_off = ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( y, datIndex );

        coeffSrcType = ePolynomeCoefficient::COEFF_T;
        coeffDependsType = ePolynomeCoefficient::COEFF_A;

        return true;
    }

    return false;
}

// Ever heard of polynomes? They are pretty cool.
// This function is used to get the polynome to calculate t depending on b in a 3D plane equation.
MATH_INLINE bool plane_solution_fetcht_find_polynome(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& coeff_resolve, ePolynomeCoefficient& coeffSrcType, ePolynomeCoefficient& coeffDependType
)
{
    // Specialized math.
    if ( ivec( x, 1 ) != 0 && ivec( x, 2 ) != 0 && ivec( x, 3 ) != 0 )
    {
        if ( plane_solution_fetcht_type_a( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType ) )
            return true;
    }

    // Always search further if no solution has been found yet.

    if ( ivec( x, 1 ) == 0 )
    {
        if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 1 ) )
            return true;

        // TODO: check special solution.
    }
            
    if ( ivec( x, 2 ) == 0 )
    {
        if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 2 ) )
            return true;

        // TODO: check special solution.
    }
            
    if ( ivec( x, 3 ) == 0 )
    {
        if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 3 ) )
            return true;

        // TODO: check special solution.
    }

    // Now check for solutions if two coordinates are zero.
    if ( ivec( x, 1 ) == 0 && ivec( x, 2 ) == 0 )
    {
        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 1, 2 ) )
            return true;
    }
    else if ( ivec( x, 2 ) == 0 && ivec( x, 3 ) == 0 )
    {
        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 2, 3 ) )
            return true;
    }
    else if ( ivec( x, 1 ) == 0 && ivec( x, 3 ) == 0 )
    {
        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 1, 3 ) )
            return true;
    }

    // Check corner-case polynomes.
    if ( plane_solution_fetcht_type_d( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 1 ) ||
         plane_solution_fetcht_type_d( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 2 ) ||
         plane_solution_fetcht_type_d( u, z, o, x, y, p, coeff_resolve, coeffSrcType, coeffDependType, 3 ) )
    {
        // Gotta hate corner cases.
        return true;
    }

    // There are no more solutions, meow.
    return false;
}

// Those are resolutions for plane co-points based on intersection between planes.
// Again we start with the most complicated solutions.
inline bool plane_solution_coresolve_type_a(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut
)
{
    // Check these conditions beforehand and branch appropriately.
    assert( ivec( u, 1 ) != 0 && ivec( u, 2 ) != 0 && ivec( u, 3 ) != 0 );

    // We have three possible solutions to this problem!
    // Gotta check all of them.

    rw::V3d po_diff = rw::sub( o, p );

    p_resolve_t mod_point_zu12 = block_trans( z, u, 1, 2 ); // MUST NOT BE ZERO
    p_resolve_t mod_point_zu23 = block_trans( z, u, 2, 3 ); // MUST NOT BE ZERO

    p_resolve_t mod_point_xu12 = block_trans( x, u, 1, 2 );
    p_resolve_t mod_point_xu23 = block_trans( x, u, 2, 3 );

    if ( mod_point_zu12 != 0 && mod_point_zu23 != 0 )
    {
        p_resolve_t major_divide = ( mod_point_xu12 / mod_point_zu12 - mod_point_xu23 / mod_point_zu23 );

        if ( major_divide != 0 )
        {
            // Got one solution.
            polyOut.raise = -( block_trans( y, u, 1, 2 ) / mod_point_zu12 - block_trans( y, u, 2, 3 ) / mod_point_zu23 ) / major_divide;
            polyOut.const_off = 
                (
                    ( block_trans( po_diff, u, 1, 2 ) / mod_point_zu12 - block_trans( po_diff, u, 2, 3 ) / mod_point_zu23 )
                ) / major_divide;

            return true;
        }
    }

    p_resolve_t mod_point_zu13 = block_trans( z, u, 1, 3 );

    p_resolve_t mod_point_xu13 = block_trans( x, u, 1, 3 );

    if ( mod_point_zu23 != 0 && mod_point_zu13 != 0 )
    {
        p_resolve_t major_divide = ( mod_point_xu23 / mod_point_zu23 - mod_point_xu13 / mod_point_zu13 );

        if ( major_divide != 0 )
        {
            // Got another solution.
            polyOut.raise = -( block_trans( y, u, 2, 3 ) / mod_point_zu23 - block_trans( y, u, 1, 3 ) / mod_point_zu13 ) / major_divide;
            polyOut.const_off =
                (
                    ( block_trans( po_diff, u, 2, 3 ) / mod_point_zu23 - block_trans( po_diff, u, 1, 3 ) / mod_point_zu13 )
                ) / major_divide;

            return true;
        }
    }

    if ( mod_point_zu12 != 0 && mod_point_zu13 != 0 )
    {
        p_resolve_t major_divide = ( mod_point_xu12 / mod_point_zu12 - mod_point_xu13 / mod_point_zu13 );

        if ( major_divide != 0 )
        {
            // Last possible solution of this type.
            polyOut.raise = -( block_trans( y, u, 1, 2 ) / mod_point_zu12 - block_trans( y, u, 1, 3 ) / mod_point_zu13 ) / major_divide;
            polyOut.const_off =
                (
                    ( block_trans( po_diff, u, 1, 2 ) / mod_point_zu12 - block_trans( po_diff, u, 1, 3 ) / mod_point_zu13 )
                ) / major_divide;

            return true;
        }
    }

    // No solution for this kind of equation here.
    return false;
}

// Less complicated solution, with a specialized context.
inline bool plane_solution_coresolve_type_b(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& dependsType,
    int datIndex
)
{
    assert( ivec( u, datIndex ) == 0 );

    rw::V3d po_diff = rw::sub( o, p );

    // Figure out the valid permutation to use, meow.
    // Unlike the hyper complex solution type_a, we only have one possible equation, meow.
    int prim_divide = ( ( datIndex == 1 ) ? 2 : 1 );
    int sec_divide = ( ( datIndex == 3 ) ? 2 : 3 );

    // Scan for one of the many possible solutions to the 3D plane intersection problem.
    // We require the solution so that we can intersect some interesting objects.

    if ( ivec( u, prim_divide ) != 0 && ivec( u, sec_divide ) != 0 )
    {
        // Try to get a solution the generic way.

        if ( ivec( z, datIndex ) != 0 )
        {
            p_resolve_t mod_point_zu = block_trans( z, u, prim_divide, sec_divide );

            if ( mod_point_zu != 0 )
            {
                p_resolve_t major_divide = ( ivec( x, datIndex ) - block_trans( x, u, prim_divide, sec_divide ) / mod_point_zu );

                if ( major_divide != 0 )
                {
                    // One solution, pls :)
                    polyOut.raise = -( ivec( y, datIndex ) / ivec( z, datIndex ) - block_trans( y, u, prim_divide, sec_divide ) / mod_point_zu ) / major_divide;
                    polyOut.const_off = 
                        (
                            ( ivec( po_diff, datIndex ) / ivec( z, datIndex ) - block_trans( po_diff, u, prim_divide, sec_divide ) / mod_point_zu )
                        ) / major_divide;

                    dependsType = ePolynomeCoefficient::COEFF_T;

                    return true;
                }
            }
        }

        // Generic way failed, go to more specialized solutions.

        p_resolve_t mod_point_xu = block_trans( x, u, prim_divide, sec_divide );

        if ( mod_point_xu != 0 )
        {
            polyOut.raise = -block_trans( y, u, prim_divide, sec_divide ) / mod_point_xu;
            polyOut.const_off = ( block_trans( po_diff, u, prim_divide, sec_divide ) ) / mod_point_xu;

            dependsType = ePolynomeCoefficient::COEFF_T;

            return true;
        }

        if ( ivec( x, prim_divide ) == 0 && ivec( x, sec_divide ) == 0 )
        {
            if ( ivec( x, datIndex ) != 0 )
            {
                p_resolve_t b = 0;  // can be any value? needs testing.

                polyOut.raise = -ivec( y, datIndex ) / ivec( x, datIndex );
                polyOut.const_off = ( b * ivec( z, datIndex ) + ivec( po_diff, datIndex ) ) / ivec( x, datIndex );

                dependsType = ePolynomeCoefficient::COEFF_T;

                return true;
            }
        }
    }

    if ( ivec( z, datIndex ) == 0 )
    {
        if ( ivec( x, datIndex ) != 0 )
        {
            polyOut.raise = -ivec( y, datIndex ) / ivec( x, datIndex );
            polyOut.const_off = ( ivec( po_diff, datIndex ) ) / ivec( x, datIndex );

            dependsType = ePolynomeCoefficient::COEFF_T;

            return true;
        }
        else
        {
            if ( ivec( u, prim_divide ) != 0 && ivec( u, sec_divide ) != 0 )
            {
                p_resolve_t divisor = block_trans( x, u, prim_divide, sec_divide );

                if ( divisor != 0 )
                {
                    p_resolve_t b = 0;  // can be any value. need to test this.

                    polyOut.raise = -block_trans( y, u, prim_divide, sec_divide ) / divisor;
                    polyOut.const_off =
                        (
                            b * block_trans( z, u, prim_divide, sec_divide )
                            +
                            block_trans( po_diff, u, prim_divide, sec_divide )
                        ) / divisor;

                    dependsType = ePolynomeCoefficient::COEFF_T;

                    return true;
                }
            }
        }
    }

    // No solution.
    return false;
}

// A tiny solution if we have a pretty simple plane relationship.
inline bool plane_solution_coresolve_type_c(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut,
    int idxPrim, int idxSec
)
{
    // Check this beforehand.
    assert( ivec( u, idxPrim ) == 0 && ivec( u, idxSec ) == 0 );

    if ( ivec( z, idxPrim ) != 0 && ivec( z, idxSec ) != 0 )
    {
        p_resolve_t divisor = block_trans( x, z, idxPrim, idxSec );

        if ( divisor != 0 )
        {
            rw::V3d po_diff = rw::sub( o, p );

            polyOut.raise = -block_trans( y, z, idxPrim, idxSec ) / divisor;
            polyOut.const_off = block_trans( po_diff, z, idxPrim, idxSec ) / divisor;

            return true;
        }
    }

    // No solution :(
    return false;
}

// Get special corner case polynomes.
MATH_INLINE bool plane_solution_coresolve_type_d(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut, ePolynomeCoefficient& dependsType,
    int datIndex
)
{
    int prim_divide = ( ( datIndex == 1 ) ? 2 : 1 );
    int sec_divide = ( ( datIndex == 3 ) ? 2 : 3 );

    if ( ivec( x, datIndex ) != 0 && ivec( x, prim_divide ) == 0 && ivec( x, sec_divide ) == 0 &&
         ivec( y, datIndex ) == 0 && ivec( z, datIndex ) == 0 && ivec( u, prim_divide ) == 0 && ivec( u, sec_divide ) == 0 &&
         ( ivec( y, prim_divide ) != 0 || ivec( z, prim_divide ) != 0 ) && ( ivec( y, sec_divide ) != 0 || ivec( z, sec_divide ) != 0 ) )
    {
        polyOut.raise = ivec( u, datIndex ) / ivec( x, datIndex );
        polyOut.const_off = ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( x, datIndex );

        dependsType = ePolynomeCoefficient::COEFF_A;

        return true;
    }

    if ( ivec( x, datIndex ) != 0 && ivec( x, prim_divide ) == 0 && ivec( x, sec_divide ) == 0 && ivec( u, datIndex ) == 0 )
    {
        polyOut.raise = ivec( z, datIndex ) / ivec( x, datIndex );
        polyOut.const_off = ( ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( x, datIndex );

        dependsType = ePolynomeCoefficient::COEFF_B;

        return true;
    }

    return false;
}

// Coresolution main routine.
MATH_INLINE bool plane_solution_coresolve(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    Simple_Polynome& polyOut,
    ePolynomeCoefficient& dependsType
)
{
    if ( ivec( u, 1 ) != 0 && ivec( u, 2 ) != 0 && ivec( u, 3 ) != 0 )
    {
        if ( plane_solution_coresolve_type_a( u, z, o, x, y, p, polyOut ) )
        {
            dependsType = ePolynomeCoefficient::COEFF_T;
            return true;
        }
    }

    if ( ivec( u, 1 ) == 0 )
    {
        if ( plane_solution_coresolve_type_b( u, z, o, x, y, p, polyOut, dependsType, 1 ) )
            return true;
    }

    if ( ivec( u, 2 ) == 0 )
    {
        if ( plane_solution_coresolve_type_b( u, z, o, x, y, p, polyOut, dependsType, 2 ) )
            return true;
    }

    if ( ivec( u, 3 ) == 0 )
    {
        if ( plane_solution_coresolve_type_b( u, z, o, x, y, p, polyOut, dependsType, 3 ) )
            return true;
    }

    if ( ivec( u, 1 ) == 0 && ivec( u, 2 ) == 0 )
    {
        if ( plane_solution_coresolve_type_c( u, z, o, x, y, p, polyOut, 1, 2 ) )
        {
            dependsType = ePolynomeCoefficient::COEFF_T;
            return true;
        }
    }

    if ( ivec( u, 1 ) == 0 && ivec( u, 3 ) == 0 )
    {
        if ( plane_solution_coresolve_type_c( u, z, o, x, y, p, polyOut, 1, 3 ) )
        {
            dependsType = ePolynomeCoefficient::COEFF_T;
            return true;
        }
    }

    if ( ivec( u, 2 ) == 0 && ivec( u, 3 ) == 0 )
    {
        if ( plane_solution_coresolve_type_c( u, z, o, x, y, p, polyOut, 2, 3 ) )
        {
            dependsType = ePolynomeCoefficient::COEFF_T;
            return true;
        }
    }

    // Special polynomial dependency situations.
    if ( plane_solution_coresolve_type_d( u, z, o, x, y, p, polyOut, dependsType, 1 ) ||
         plane_solution_coresolve_type_d( u, z, o, x, y, p, polyOut, dependsType, 2 ) ||
         plane_solution_coresolve_type_d( u, z, o, x, y, p, polyOut, dependsType, 3 ) )
    {
        // We found some corner case bs.
        return true;
    }

    // No co-point. Should not happen, really.
    return false;
}

inline bool is_intersecting_unit_region( p_resolve_t a, p_resolve_t b )
{
    if ( a > 1 && b > 1 || a < 0 && b < 0 )
        return false;

    return true;
}

inline bool is_line_intersecting_line( p_resolve_t s, p_resolve_t b )
{
    return s >= 0 && s <= 1 && b >= 0 && b <= 1;
}

inline bool plane_solution_standard_intersection(
    p_resolve_t s0, p_resolve_t t0,
    p_resolve_t s1, p_resolve_t t1
)
{
    // Cut it with the unit cube.
    p_resolve_t x_off = ( s1 - s0 );
    p_resolve_t y_off = ( t1 - t0 );

    p_resolve_t x_pos = s0;
    p_resolve_t y_pos = t0;

    // If any point is inside the unit cube, we are OK.
    if ( s0 >= 0 && s0 <= 1 && t0 >= 0 && t0 <= 1 || s1 >= 0 && s1 <= 1 && t1 >= 0 && t1 <= 1 )
        return true;

    if ( x_off != 0 )
    {
        // Check left side.
        {
            p_resolve_t s = -x_pos / x_off;

            p_resolve_t b = ( s * y_off + y_pos );

            if ( is_line_intersecting_line( s, b ) )
                return true;
        }

        // Check right side.
        {
            p_resolve_t s = ( 1 - x_pos ) / x_off;

            p_resolve_t b = ( s * y_off + y_pos );

            if ( is_line_intersecting_line( s, b ) )
                return true;
        }
    }
    else
    {
        if ( x_pos < 0 || x_pos > 1 )
            return false;
    }

    if ( y_off != 0 )
    {
        // Check bottom side.
        {
            p_resolve_t s = -y_pos / y_off;

            p_resolve_t a = ( s * x_off + x_pos );

            if ( is_line_intersecting_line( s, a ) )
                return true;
        }
#
        // Check top side.
        {
            p_resolve_t s = ( 1 - y_pos ) / y_off;

            p_resolve_t a = ( s * x_off + x_pos );

            if ( is_line_intersecting_line( s, a ) )
                return true;
        }
    }
    else
    {
        if ( y_pos < 0 || y_pos > 1 )
            return false;
    }

    return false;
}

inline bool is_point_inside_unit_triangle( p_resolve_t s, p_resolve_t t )
{
    return ( s >= 0 && t >= 0 && s + t <= 1 );
}

// When we intersect two planes, we get two points that are the maximal intersection points of a standard plane.
// We want to limit those to the points that are given by the actual shape of this plane.
// In this case, the plane has the shape of a triangle.
inline bool plane_solution_triangle_delimiter(
    p_resolve_t s0, p_resolve_t t0, p_resolve_t s1, p_resolve_t t1,
    p_resolve_t& new_s0, p_resolve_t& new_t0, p_resolve_t& new_s1, p_resolve_t& new_t1
)
{
    p_resolve_t x_off = ( s1 - s0 );
    p_resolve_t y_off = ( t1 - t0 );

    p_resolve_t x_pos = s0;
    p_resolve_t y_pos = t0;

    // Check all side that the plane could intersect the triangle with and return appropriate points.
    bool intersects_left_side = false;
    bool intersects_bottom_side = false;
    bool intersects_triangle_side = false;

    p_resolve_t left_side_intersect_s, left_side_intersect_t;
    p_resolve_t bottom_side_intersect_s, bottom_side_intersect_t;
    p_resolve_t triangle_side_intersect_s, triangle_side_intersect_t;

    if ( x_off != 0 )
    {
        // Check left side.
        {
            p_resolve_t s = -x_pos / x_off;

            p_resolve_t b = ( s * y_off + y_pos );

            if ( is_line_intersecting_line( s, b ) )
            {
                intersects_left_side = true;

                left_side_intersect_s = s * x_off + x_pos;
                left_side_intersect_t = s * y_off + y_pos;
            }
        }
    }

    if ( y_off != 0 )
    {
        // Check bottom side.
        {
            p_resolve_t s = ( -y_pos ) / y_off;

            p_resolve_t a = ( s * x_off + x_pos );

            if ( is_line_intersecting_line( s, a ) )
            {
                intersects_bottom_side = true;

                bottom_side_intersect_s = s * x_off + x_pos;
                bottom_side_intersect_t = s * y_off + y_pos;
            }
        }
    }

    // And now for the triangle side.
    {
        p_resolve_t divisor = ( x_off + y_off );

        if ( divisor != 0 )
        {
            p_resolve_t s = ( 1 - ( x_pos + y_pos ) ) / divisor;

            p_resolve_t t = s * x_off + x_pos;

            if ( is_line_intersecting_line( s, t ) )
            {
                intersects_triangle_side = true;

                triangle_side_intersect_s = t;
                triangle_side_intersect_t = s * y_off + y_pos;
            }
        }
    }

    // Check if any point is inside the triangle.
    bool is_zero_point_inside = is_point_inside_unit_triangle( s0, t0 );
    bool is_one_point_inside = is_point_inside_unit_triangle( s1, t1 );

    // Delimit the points.
    bool has_first_point = false;

    if ( is_zero_point_inside )
    {
        new_s0 = s0;
        new_t0 = t0;

        has_first_point = true;
    }
    
    if ( is_one_point_inside )
    {
        if ( has_first_point )
        {
            new_s1 = s1;
            new_t1 = t1;

            return true;
        }

        new_s0 = s1;
        new_t0 = t1;

        has_first_point = true;
    }

    // Now we just go through each side and check for intersection.
    // The intersections have to amount to max two additional points.

    if ( intersects_left_side )
    {
        if ( !has_first_point || ( new_s0 != left_side_intersect_s || new_t0 != left_side_intersect_t ) )
        {
            if ( has_first_point )
            {
                new_s1 = left_side_intersect_s;
                new_t1 = left_side_intersect_t;

                return true;
            }

            new_s0 = left_side_intersect_s;
            new_t0 = left_side_intersect_t;

            has_first_point = true;
        }
    }

    if ( intersects_bottom_side )
    {
        if ( !has_first_point || ( new_s0 != bottom_side_intersect_s || new_t0 != bottom_side_intersect_t ) )
        {
            if ( has_first_point )
            {
                new_s1 = bottom_side_intersect_s;
                new_t1 = bottom_side_intersect_t;

                return true;
            }

            new_s0 = bottom_side_intersect_s;
            new_t0 = bottom_side_intersect_t;

            has_first_point = true;
        }
    }

    if ( intersects_triangle_side )
    {
        if ( !has_first_point || ( new_s0 != triangle_side_intersect_s || new_t0 != triangle_side_intersect_t ) )
        {
            if ( has_first_point )
            {
                new_s1 = triangle_side_intersect_s;
                new_t1 = triangle_side_intersect_t;

                return true;
            }

            new_s0 = triangle_side_intersect_s;
            new_t0 = triangle_side_intersect_t;

            has_first_point = true;
        }
    }

    // If we have one intersection point, then the other gotta be outside.
    if ( has_first_point )
    {
        if ( !is_zero_point_inside )
        {
            new_s1 = s0;
            new_t1 = t0;
            return true;
        }

        if ( !is_one_point_inside )
        {
            new_s1 = s1;
            new_t1 = t1;
            return true;
        }

        // Something can always go wrong, I guess.
    }

    // We are not properly intersecting.
    return false;
}

inline bool do_segments_intersect( p_resolve_t a_min, p_resolve_t a_max, p_resolve_t b_min, p_resolve_t b_max )
{
    // We can easily say if segments do _not_ intersect.
    p_resolve_t real_min_a = std::min( a_min, a_max );

    if ( b_min < real_min_a && b_max < real_min_a )
        return false;

    p_resolve_t real_max_a = std::max( a_min, a_max );

    if ( b_min > real_max_a && b_max > real_max_a )
        return false;

    return true;
}

template <typename delimiterCallbackThis, typename delimiterCallbackRight>
MATH_INLINE bool plane_solution_solve_intersection(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    delimiterCallbackThis& delim_cb_this, delimiterCallbackRight& delim_cb_right
)
{
    // Try some advanced math.
    // I hope I got all cases of plane-plane intersection figured out.

    // Legend: x is vector a of this, s is scalar
    //         y is vector b of this, t is scalar
    //         u is vector a of right, a is scalar
    //         z is vector b of right, b is scalar
    //         p is position of this
    //         o is position of right

    bool hasIntersection = false;

    // Check all possible cases.
    // We first do get the value of t depending on b.
    // Then we resolve s depending on t, resolve a depending on b.
    Simple_Polynome t_resolve;
    bool hasPolynomeForT = false;   // true if we found a solution for t.
    ePolynomeCoefficient t_resolve_srcCoeff, t_resolve_dependsCoeff;
    {
        // If we cannot find ANY polynome for b, then there cannot be an intersection anyway.
        hasPolynomeForT = plane_solution_fetcht_find_polynome( u, z, o, x, y, p, t_resolve, t_resolve_srcCoeff, t_resolve_dependsCoeff );

        if ( hasPolynomeForT )
        {
            assert( t_resolve_srcCoeff == ePolynomeCoefficient::COEFF_T );
        }
    }

    // Try to find a polynome for s.
    Simple_Polynome s_resolve;
    bool hasPolynomeForS = false;
    ePolynomeCoefficient s_resolve_srcCoeff, s_resolve_dependsCoeff;
    {
        // Kind of a clever trick here, meow. We swapped x and y because transforming would be same anyway.
        hasPolynomeForS = plane_solution_fetcht_find_polynome( u, z, o, y, x, p, s_resolve, s_resolve_srcCoeff, s_resolve_dependsCoeff );

        if ( hasPolynomeForS )
        {
            assert( s_resolve_srcCoeff == ePolynomeCoefficient::COEFF_T );

            s_resolve_srcCoeff = ePolynomeCoefficient::COEFF_S;
        }
    }

    // Do we have a polynome for t and s?
    if ( hasPolynomeForT && hasPolynomeForS )
    {
        // Intersection only makes sense if we actually HAVE something to intersect by.
        if ( t_resolve.IsConst() == true && s_resolve.IsConst() == true )
            return false;   // Nothing is really intersecting.

        // If both are not constant, we limit ourselves to intersecting if both depend on the same variable.
        if ( t_resolve.IsConst() == false && s_resolve.IsConst() == false && s_resolve_dependsCoeff != t_resolve_dependsCoeff )
            return false;   // maybe later.

        // This is the section on the right plane to check intersection against.
        p_resolve_t p0 = 0;
        p_resolve_t p1 = 1;

        ePolynomeCoefficient p_coeff;

        // Figure out how to go about this case of intersection.
        if ( t_resolve.IsConst() == false )
        {
            p_coeff = t_resolve_dependsCoeff;
        }

        if ( s_resolve.IsConst() == false )
        {
            p_coeff = s_resolve_dependsCoeff;
        }

        // Get intersection points.
        p_resolve_t t0, t1;

        t0 = t_resolve.Calculate( p0 );
        t1 = t_resolve.Calculate( p1 );
        
        // The other thing gotta be constant or depend on the same thing.
        p_resolve_t s0, s1;

        s0 = s_resolve.Calculate( p0 );
        s1 = s_resolve.Calculate( p1 );

        // Resolve for local points on the planes.

        p_resolve_t new_s0, new_t0, new_s1, new_t1;

        bool could_delimit_plane_this = false;
        {
            // And finally do the standard intersection.
            could_delimit_plane_this = delim_cb_this( s0, t0, s1, t1, new_s0, new_t0, new_s1, new_t1 );
        }

        if ( could_delimit_plane_this )
        {
            // We could intersect this plane!
            // Need to check the other too, tho.

            // Need a way to get the co-dependency polynome for p0, p1.
            Simple_Polynome co_polynome;
            ePolynomeCoefficient co_polynome_depends, co_polynome_srcType;
            
            // If we set the b coefficient, we need a way to get a.
            // If we set the a coefficient, we need a way to get b.
            bool has_co_solution;
            {
                if ( p_coeff == ePolynomeCoefficient::COEFF_A )
                {
                    //has_co_solution = plane_solution_fetcht_find_polynome( x, y, p, u, z, o, co_polynome, co_polynome_srcType, co_polynome_depends );
                    has_co_solution = plane_solution_coresolve( x, y, p, u, z, o, co_polynome, co_polynome_depends );

                    // Correctly translate it.
                    co_polynome_srcType = ePolynomeCoefficient::COEFF_B;
                }
                else if ( p_coeff == ePolynomeCoefficient::COEFF_B )
                {
                    // Again the swap trick.
                    has_co_solution = plane_solution_fetcht_find_polynome( x, y, p, z, u, o, co_polynome, co_polynome_srcType, co_polynome_depends );

                    // Correctly translate it.
                    assert( co_polynome_srcType == ePolynomeCoefficient::COEFF_T );

                    co_polynome_srcType = ePolynomeCoefficient::COEFF_A;

                    if ( co_polynome_depends == ePolynomeCoefficient::COEFF_A )
                    {
                        co_polynome_depends = ePolynomeCoefficient::COEFF_T;
                    }
                    else if ( co_polynome_depends == ePolynomeCoefficient::COEFF_B )
                    {
                        co_polynome_depends = ePolynomeCoefficient::COEFF_S;
                    }
                    else
                    {
                        assert( 0 );
                        return false;
                    }
                }
                else
                {
                    // Some sort of error.
                    assert( 0 );
                    return false;
                }
            }

            if ( has_co_solution )
            {
                // Resolve the co-points.

                p_resolve_t co0, co1;

                if ( co_polynome_depends == ePolynomeCoefficient::COEFF_S )
                {
                    co0 = co_polynome.Calculate( s0 );
                    co1 = co_polynome.Calculate( s1 );
                }
                else if ( co_polynome_depends == ePolynomeCoefficient::COEFF_A ||
                          co_polynome_depends == ePolynomeCoefficient::COEFF_B )
                {
#if 0
                    if ( p_coeff != co_polynome_depends )
                        return false;
#endif

                    co0 = co_polynome.Calculate( p0 );
                    co1 = co_polynome.Calculate( p1 );
                }
                else if ( co_polynome_depends == ePolynomeCoefficient::COEFF_T )
                {
                    co0 = co_polynome.Calculate( t0 );
                    co1 = co_polynome.Calculate( t1 );
                }
                else
                {
                    assert( 0 );
                    return false;
                }

                p_resolve_t new_p0, new_p1, new_co0, new_co1;
                bool could_delimit_plane_right = false;
                {
                    // Now get the real points on the plane.
                    could_delimit_plane_right = delim_cb_right( p0, co0, p1, co1, new_p0, new_co0, new_p1, new_co1 );
                }

                if ( could_delimit_plane_right )
                {
                    // Now check if the segments actually intersect each other.
                    // This actually works because our planes are expected to be enclosed (~convex).
                    p_resolve_t t_min = t_resolve.Calculate( new_p0 );
                    p_resolve_t t_max = t_resolve.Calculate( new_p1 );

                    if ( do_segments_intersect( new_t0, new_t1, t_min, t_max ) )
                        return true;
                }
            }
        }
    }

    // No intersection :(
    return false;
}

bool Plane::intersectWith( const Plane& right ) const
{
    rw::V3d x = this->a;
    rw::V3d y = this->b;
    rw::V3d u = right.a;
    rw::V3d z = right.b;
    rw::V3d p = this->pos;
    rw::V3d o = right.pos;

    return plane_solution_solve_intersection(
        u, z, o, x, y, p,
        []( p_resolve_t s0, p_resolve_t t0, p_resolve_t s1, p_resolve_t t1, p_resolve_t& new_s0, p_resolve_t& new_t0, p_resolve_t& new_s1, p_resolve_t& new_t1 )
    {
        if ( plane_solution_standard_intersection( s0, t0, s1, t1 ) == false )
            return false;

        new_s0 = s0;
        new_t0 = t0;
        new_s1 = s1;
        new_t1 = t1;
        return true;
    },
        []( p_resolve_t s0, p_resolve_t t0, p_resolve_t s1, p_resolve_t t1, p_resolve_t& new_s0, p_resolve_t& new_t0, p_resolve_t& new_s1, p_resolve_t& new_t1 )
    {
        if ( plane_solution_standard_intersection( s0, t0, s1, t1 ) == false )
            return false;

        new_s0 = s0;
        new_t0 = t0;
        new_s1 = s1;
        new_t1 = t1;
        return true;
    });
}

bool TrianglePlane::intersectWith( const TrianglePlane& right ) const
{
    rw::V3d x = this->a;
    rw::V3d y = this->b;
    rw::V3d u = right.a;
    rw::V3d z = right.b;
    rw::V3d p = this->pos;
    rw::V3d o = right.pos;

    return plane_solution_solve_intersection(
        u, z, o, x, y, p,
        []( p_resolve_t s0, p_resolve_t t0, p_resolve_t s1, p_resolve_t t1, p_resolve_t& new_s0, p_resolve_t& new_t0, p_resolve_t& new_s1, p_resolve_t& new_t1 )
    {
        return plane_solution_triangle_delimiter( s0, t0, s1, t1, new_s0, new_t0, new_s1, new_t1 );
    },
        []( p_resolve_t s0, p_resolve_t t0, p_resolve_t s1, p_resolve_t t1, p_resolve_t& new_s0, p_resolve_t& new_t0, p_resolve_t& new_s1, p_resolve_t& new_t1 )
    {
        return plane_solution_triangle_delimiter( s0, t0, s1, t1, new_s0, new_t0, new_s1, new_t1 );
    });
}

inline bool isLinearIndependent( const rw::V3d& one, const rw::V3d& two )
{
    rw::Matrix3 testMat;
    testMat.right = one;
    testMat.up = two;
    testMat.at = rw::cross( one, two );

    return ( testMat.determinant() != 0 );
}

inline bool verifyPlanePointConfiguration(
    const rw::V3d& one, const rw::V3d& two, const rw::V3d& three, const rw::V3d& four
)
{
    rw::V3d oneVec = rw::sub( two, one );
    rw::V3d twoVec = rw::sub( three, one );
    rw::V3d threeVec = rw::sub( four, one );

    // verify that the determinant of this 3x3 matrix is zero.
    rw::Matrix3 planeMat;
    planeMat.right = oneVec;
    planeMat.at = twoVec;
    planeMat.up = threeVec;

    auto det = planeMat.determinant();

    if ( det != 0 )
    {
        return false;
    }

    // verify that we atleast have two dimensions.
    bool hasTwoDimms = false;
    {
        if ( isLinearIndependent( oneVec, twoVec ) ||
             isLinearIndependent( oneVec, threeVec ) ||
             isLinearIndependent( twoVec, threeVec ) )
        {
            hasTwoDimms = true;
        }
    }

    return hasTwoDimms;
}

inline bool setupQuaderMatrices( const Quader& in, rw::Matrix& inv_bottomMat, rw::Matrix& inv_topMat )
{
    // Idea: brl - bottom rear left
    //       tfl - top front left
    // to map eight points of a Quader (just imagine a standard cube).

    rw::Matrix bottomMat;
    bottomMat.right = rw::sub( in.brr, in.brl );
    bottomMat.rightw = 0;
    bottomMat.up = rw::sub( in.trl, in.brl );
    bottomMat.upw = 0;
    bottomMat.at = rw::sub( in.bfl, in.brl );
    bottomMat.atw = 0;
    bottomMat.pos = in.brl;
    bottomMat.posw = 1;

    rw::Matrix topMat;
    topMat.right = rw::sub( in.tfl, in.tfr );
    topMat.rightw = 0;
    topMat.up = rw::sub( in.bfr, in.tfr );
    topMat.upw = 0;
    topMat.at = rw::sub( in.trr, in.tfr );
    topMat.atw = 0;
    topMat.pos = in.tfr;
    topMat.posw = 1;

    // We are using a really shit math library for the moment.
    // Looks like copy paste to me.
    // REMEMBER: out is first argument, in is second.
    {
        rw::bool32 couldInvert = rw::Matrix::invert( &inv_bottomMat, &bottomMat );

        if ( couldInvert == 0 )
        {
            return false;
        }
    }

    {
        rw::bool32 couldInvert = rw::Matrix::invert( &inv_topMat, &topMat );

        if ( couldInvert == 0 )
        {
            return false;
        }
    }

    return true;
}

inline bool calculatePlaneAdjacencyMatrices(
    rw::V3d bl, rw::V3d br, rw::V3d tl, rw::V3d tr,
    rw::Matrix& inv_planeSpace_bottom, rw::Matrix& inv_planeSpace_top
)
{
    rw::V3d prim_top = rw::sub( tl, bl );
    rw::V3d prim_right = rw::sub( br, bl );

    rw::V3d sec_top = rw::sub( br, tr );
    rw::V3d sec_right = rw::sub( tl, tr );

    // Calculate plane normal.
    // We should use the primary vectors to get a normal that points outside the Quader.
    rw::V3d planeNormal = rw::normalize( rw::cross( prim_right, prim_top ) );

    // Set up plane space matrices.
    rw::Matrix planeSpace_bottom;
    planeSpace_bottom.right = prim_right;
    planeSpace_bottom.rightw = 0;
    planeSpace_bottom.at = prim_top;
    planeSpace_bottom.atw = 0;
    planeSpace_bottom.up = planeNormal;
    planeSpace_bottom.upw = 0;
    planeSpace_bottom.pos = bl;
    planeSpace_bottom.posw = 1;

    rw::Matrix planeSpace_top;
    planeSpace_top.right = sec_right;
    planeSpace_top.rightw = 0;
    planeSpace_top.at = sec_top;
    planeSpace_top.atw = 0;
    planeSpace_top.up = planeNormal;
    planeSpace_top.upw = 0;
    planeSpace_top.pos = tr;
    planeSpace_top.posw = 1;

    // Calculate the inverse of matrices that check for adjacency.
    {
        rw::bool32 couldInvert = rw::Matrix::invert( &inv_planeSpace_bottom, &planeSpace_bottom );

        if ( couldInvert == 0 )
        {
            return false;
        }
    }

    {
        rw::bool32 couldInvert = rw::Matrix::invert( &inv_planeSpace_top, &planeSpace_top );

        if ( couldInvert == 0 )
        {
            return false;
        }
    }

    return true;
}

inline Quader::Quader(
    rw::V3d brl, rw::V3d brr, rw::V3d bfl, rw::V3d bfr,
    rw::V3d trl, rw::V3d trr, rw::V3d tfl, rw::V3d tfr
) : brl( brl ), brr( brr ), bfl( bfl ), bfr( bfr ),
    trl( trl ), trr( trr ), tfl( tfl ), tfr( tfr )
{
    // Verify that we really make up a quader!
#ifdef USE_MATH_VERIFICATION
    assert( verifyPlanePointConfiguration( brl, brr, trl, trr ) == true );  // rear plane
    assert( verifyPlanePointConfiguration( bfl, brl, tfl, trl ) == true );  // left plane
    assert( verifyPlanePointConfiguration( bfr, bfl, tfr, tfl ) == true );  // front plane
    assert( verifyPlanePointConfiguration( brr, bfr, trr, tfr ) == true );  // right plane
    assert( verifyPlanePointConfiguration( brr, brl, bfr, bfl ) == true );  // bottom plane
    assert( verifyPlanePointConfiguration( trr, trl, tfr, tfl ) == true );  // top plane
#endif

    // I trust the programmer that he knows how to select 8 valid points for a Quader.
    // If we really cannot into that, then fml.
    // The above logic should fault if there is a problem.

    // We want to precalculate some things so that we can use this object very fast multiple times.
    bool calculateRearPlaneMatrices = calculatePlaneAdjacencyMatrices( brl, brr, trl, trr, this->rearPlane_inv_planeSpace_bottom, this->rearPlane_inv_planeSpace_top );
    bool calculateLeftPlaneMatrices = calculatePlaneAdjacencyMatrices( bfl, brl, tfl, trl, this->leftPlane_inv_planeSpace_bottom, this->leftPlane_inv_planeSpace_top );
    bool calculateFrontPlaneMatrices = calculatePlaneAdjacencyMatrices( bfr, bfl, tfr, tfl, this->frontPlane_inv_planeSpace_bottom, this->frontPlane_inv_planeSpace_top );
    bool calculateRightPlaneMatrices = calculatePlaneAdjacencyMatrices( brr, bfr, trr, tfr, this->rightPlane_inv_planeSpace_bottom, this->rightPlane_inv_planeSpace_bottom );
    bool calculateBottomPlaneMatrices = calculatePlaneAdjacencyMatrices( brr, brl, bfr, bfl, this->bottomPlane_inv_planeSpace_bottom, this->bottomPlane_inv_planeSpace_top );
    bool calculateTopPlaneMatrices = calculatePlaneAdjacencyMatrices( trr, trl, tfr, tfl, this->topPlane_inv_planeSpace_bottom, this->topPlane_inv_planeSpace_top );

    // Should actually work if the planes actually are planes.
    assert( calculateRearPlaneMatrices == true );
    assert( calculateLeftPlaneMatrices == true );
    assert( calculateFrontPlaneMatrices == true );
    assert( calculateRightPlaneMatrices == true );
    assert( calculateBottomPlaneMatrices == true );
    assert( calculateTopPlaneMatrices == true );

    // Also calculate matrices for determining of points are inside the Quader.
    bool couldCalculateLocalSpaceMatrices = setupQuaderMatrices( *this, this->inv_localSpace_bottom, this->inv_localSpace_top );

    assert( couldCalculateLocalSpaceMatrices == true );
}

inline bool isPointInNegativeSpace( const rw::V3d& point, bool inclusive )
{
    if ( !inclusive )
    {
        return ( point.x <= 0 || point.y <= 0 || point.z <= 0 );
    }

    return ( point.x < 0 || point.y < 0 || point.z < 0 );
}

inline bool isPointInInclusionSpace(
    const rw::Matrix& inv_bottomMat, const rw::Matrix& inv_topMat,
    const rw::V3d& point, bool inclusive
)
{
    // Do exclusion checks. What is in-between must be our thing.
    // *bottom plane.
    {
        rw::V3d localSpace_bottom = ((rw::Matrix&)inv_bottomMat).transPoint( point );

        if ( isPointInNegativeSpace( localSpace_bottom, inclusive ) )
            return false;
    }

    // top plane.
    {
        rw::V3d localSpace_top = ((rw::Matrix&)inv_topMat).transPoint( point );

        if ( isPointInNegativeSpace( localSpace_top, inclusive ) )
            return false;
    }

    // Well, I guess we are inside.
    return true;
}

bool Quader::isPointInside( const rw::V3d& point ) const
{
    // Check whether the point is inside of our thing.
    // Should be a simple math thing.

    return isPointInInclusionSpace( this->inv_localSpace_bottom, this->inv_localSpace_top, point, true );
}

MATH_INLINE void make_quader_plane_triangles(
    const rw::V3d& bl, const rw::V3d& br, const rw::V3d& tl, const rw::V3d& tr,
    TrianglePlane& bottom, TrianglePlane& top
)
{
    bottom.pos = bl;
    bottom.a = rw::sub( br, bl );
    bottom.b = rw::sub( tl, bl );

    top.pos = tr;
    top.a = rw::sub( tl, tr );
    top.b = rw::sub( br, tr );
}

inline bool isQuaderPlaneIntersectingPlane(
    const rw::V3d& bl, const rw::V3d& br, const rw::V3d& tl, const rw::V3d& tr,
    const rw::V3d& int_bl, const rw::V3d& int_br, const rw::V3d& int_tl, const rw::V3d& int_tr
)
{
    // One plane is two triangles.
    TrianglePlane prim_bottom, prim_top,
                  sec_bottom, sec_top;

    make_quader_plane_triangles( bl, br, tl, tr, prim_bottom, prim_top );
    make_quader_plane_triangles( int_bl, int_br, int_tl, int_tr, sec_bottom, sec_top );

    // Check for intersections.
    return ( prim_bottom.intersectWith( sec_bottom ) || prim_bottom.intersectWith( sec_top ) || prim_top.intersectWith( sec_bottom ) || prim_top.intersectWith( sec_top ) );
}

inline bool intersectQuaderPlaneWithQuader(
    const rw::V3d& bl, const rw::V3d& br, const rw::V3d& tl, const rw::V3d& tr,
    const Quader& right
)
{
    return
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.brl, right.brr, right.trl, right.trr ) == true ) ||  // rear plane
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.bfl, right.brl, right.tfl, right.trl ) == true ) ||  // left plane
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.bfr, right.bfl, right.tfr, right.tfl ) == true ) ||  // front plane
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.brr, right.bfr, right.trr, right.tfr ) == true ) ||  // right plane
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.brr, right.brl, right.bfr, right.bfl ) == true ) ||  // bottom plane
        ( isQuaderPlaneIntersectingPlane( bl, br, tl, tr, right.trr, right.trl, right.tfr, right.tfl ) == true );    // top plane
}

bool Quader::intersectWith( const Quader& right ) const
{
    // This routine is used to intersect the camera frustum with bounding boxes correctly.
    // It should be used sparingly and under constant-load conditions, that is in combination with a quad-tree or octree.
    // It is optimized for speed and correctness.

    // Either the planes of the quaders intersect or one quader is inside another.
    // So lets check for that.

    // Each quader plane is made up of two triangles.
    // Kinda ugly but we gotta do that.
    {
        if (
            intersectQuaderPlaneWithQuader( this->brl, this->brr, this->trl, this->trr, right ) ||
            intersectQuaderPlaneWithQuader( this->bfl, this->brl, this->tfl, this->trl, right ) ||
            intersectQuaderPlaneWithQuader( this->bfr, this->bfl, this->tfr, this->tfl, right ) ||
            intersectQuaderPlaneWithQuader( this->brr, this->bfr, this->trr, this->tfr, right ) ||
            intersectQuaderPlaneWithQuader( this->brr, this->brl, this->bfr, this->bfl, right ) ||
            intersectQuaderPlaneWithQuader( this->trr, this->trl, this->tfr, this->tfl, right ) )
        {
            return true;
        }
    }

    // A point could be inside.
    // Any can be checked.
    {
        if ( this->isPointInside( right.brl ) )
        {
            return true;
        }
    }

    // Not intersecting.
    return false;
}

inline bool isQuaderConfigurationIntersectingLocalSpace(
    const rw::Matrix& inv_bottomMat, const rw::Matrix& inv_topMat,
    const Quader& right, bool inclusive
)
{
    if ( isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.bfl, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.bfr, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.brl, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.brr, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.tfl, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.tfr, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.trl, inclusive ) == true ||
         isPointInInclusionSpace( inv_bottomMat, inv_topMat, right.trr, inclusive ) == true )
    {
        return true;
    }

    return false;
}

inline double getPointToPointDistanceSquared( const rw::V3d& left, const rw::V3d& right )
{
    double x_dist = ( left.x - right.x );
    double y_dist = ( left.y - right.y );
    double z_dist = ( left.z - right.z );

    return ( x_dist * x_dist + y_dist * y_dist + z_dist * z_dist );
}

inline bool isPointInSphereSquared( const rw::V3d& left, const rw::V3d& right, double radiusSquared )
{
    double distSquared = getPointToPointDistanceSquared( left, right );

    return ( distSquared < radiusSquared );
}

inline bool cached_calculateSpherePlaneAdjacencyIntersect(
    const rw::Matrix& inv_planeSpace_bottom, const rw::Matrix& inv_planeSpace_top,
    rw::V3d spherePos, float radius,
    bool& result
)
{
    // Get the distance of the sphere center to the plane.
    rw::V3d localSpace_bottom = ((rw::Matrix&)inv_planeSpace_bottom).transPoint( spherePos );
    rw::V3d localSpace_top = ((rw::Matrix&)inv_planeSpace_top).transPoint( spherePos );

    // That should be same if we have a valid Quader.
    //assert( localSpace_bottom.y == localSpace_top.y );

    // REMEMBER: x maps to right, y maps to up, z maps to at.

    // Check that we are adjacent.
    if ( localSpace_bottom.x < 0 || localSpace_bottom.z < 0 ||
         localSpace_top.x < 0 || localSpace_top.z < 0 )
    {
        // We are not adjacent.
        return false;
    }

    // Check that we are above the plane.
    if ( localSpace_bottom.y < 0 )
    {
        // We could be inside the Quader or on the other side of it.
        return false;
    }

    // We intersect the plane if the absolute distance to the plane is smaller than the radius.
    result = ( localSpace_bottom.y < radius );
    return true;
}

inline bool calculateSpherePlaneAdjacencyIntersect(
    rw::V3d bl, rw::V3d br, rw::V3d tl, rw::V3d tr,
    rw::V3d spherePos, float radius,
    bool& result
)
{
    rw::Matrix inv_planeSpace_bottom, inv_planeSpace_top;

    bool couldCalculate = calculatePlaneAdjacencyMatrices(
        bl, br, tl, tr,
        inv_planeSpace_bottom, inv_planeSpace_top
    );

    if ( !couldCalculate )
        return false;   // we could not calculate matrix inverses.

    return cached_calculateSpherePlaneAdjacencyIntersect(
        inv_planeSpace_bottom, inv_planeSpace_top,
        spherePos, radius,
        result
    );
}

inline bool intersectSphereWithLine(
    rw::V3d lineDir, rw::V3d linePos, rw::V3d sphereCenter, float radius,
    double& first_intersect, double& sec_intersect
)
{
    rw::V3d off_lp_sc = rw::sub( sphereCenter, linePos );

    double lineDirLenSq = ( lineDir.x * lineDir.x + lineDir.y * lineDir.y + lineDir.z + lineDir.z );

    if ( lineDirLenSq == 0 )
        return false;

    double alpha = ( lineDir.x * ( off_lp_sc.x ) + lineDir.y * ( off_lp_sc.y ) + lineDir.z * ( off_lp_sc.z ) ) / lineDirLenSq;

    double beta = ( off_lp_sc.x * off_lp_sc.x + off_lp_sc.y * off_lp_sc.y + off_lp_sc.z * off_lp_sc.z );

    double modifier = ( ( radius * radius - beta ) / lineDirLenSq + alpha * alpha );

    if ( modifier < 0 )
        return false;

    // Those are the intersection spots.
    double sqrt_mod = sqrt( modifier );

    first_intersect = sqrt_mod + alpha;
    sec_intersect = -sqrt_mod + alpha;

    // We do intersect.
    return true;
}

inline bool checkQuaderStrutIntersection(
    rw::V3d strutStart, rw::V3d strutEnd,
    rw::V3d spherePos, float radius
)
{
    rw::V3d strutDir = rw::sub( strutEnd, strutStart );

    double first_intersect, sec_intersect;

    bool doIntersect =
        intersectSphereWithLine(
            strutDir, strutStart,
            spherePos, radius,
            first_intersect, sec_intersect
        );

    if ( !doIntersect )
        return false;

    // Check if it really did intersect.
    return ( first_intersect >= 0 && first_intersect <= 1 ||
             sec_intersect >= 0 && sec_intersect <= 1 );
}

bool Quader::intersectWith( const Sphere& right ) const
{
    // Sphere and Quader intersection is a very complicated process.
    // That is why its a really good idea to thread the frustum checking of a camera.

    // NOTE that this function has been optimized so that no matrix inversion takes place.
    // It should be quite okay to run it single threaded. :)

    // It is important to determine where exactly the sphere is.
    const rw::V3d spherePos = right.point;

    rw::V3d localSpace_bottom = ((rw::Matrix&)this->inv_localSpace_bottom).transPoint( spherePos );
    rw::V3d localSpace_top = ((rw::Matrix&)this->inv_localSpace_top).transPoint( spherePos );

    // Is the sphere center point inside the Quader?
    if ( isPointInNegativeSpace( localSpace_bottom, true ) == false && isPointInNegativeSpace( localSpace_top, true ) == false )
    {
        // A sphere whose point is inside the Quader is definitely intersecting.
        return true;
    }

    const float radius = right.radius;

    // Check for adjacency to any side of the Quader.
    {
        bool result;

        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //rear
                 this->rearPlane_inv_planeSpace_bottom, this->rearPlane_inv_planeSpace_top,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the rear plane.
            return result;
        }

        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //left
                 this->leftPlane_inv_planeSpace_bottom, this->leftPlane_inv_planeSpace_top,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the left plane.
            return result;
        }

        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //front
                 this->frontPlane_inv_planeSpace_bottom, this->frontPlane_inv_planeSpace_top,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the front plane.
            return result;
        }

        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //right
                 this->rightPlane_inv_planeSpace_bottom, this->rightPlane_inv_planeSpace_bottom,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the right plane.
            return result;
        }
             
        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //top
                 this->topPlane_inv_planeSpace_top, this->topPlane_inv_planeSpace_bottom,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the top plane.
            return result;
        }
             
        if ( cached_calculateSpherePlaneAdjacencyIntersect(    //bottom
                 this->bottomPlane_inv_planeSpace_top, this->bottomPlane_inv_planeSpace_bottom,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the bottom plane.
            return result;
        }
    }

    // Since we are a three dimensional Quader, we could intersect the struts of the Quader aswell.
    //
    //     -------------
    //     |           |
    // --> |           |
    // --> |           |
    //     |           |
    //     -------------
    // for example.
    // There are twelve struts to check against.
    //
    // Question: how far away are two parallel lines from each other?
    //           and how does this distance relate to a specific segment of the line the sphere is on?
    {
        // Struts of the top plane.
        if ( checkQuaderStrutIntersection( this->trl, this->trr, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->trr, this->tfr, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->tfr, this->tfl, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->tfl, this->trl, spherePos, radius ) )
        {
            return true;
        }

        // Struts of the bottom plane.
        if ( checkQuaderStrutIntersection( this->brl, this->brr, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->brr, this->bfr, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->bfr, this->bfl, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->bfl, this->brl, spherePos, radius ) )
        {
            return true;
        }

        // Struts connecting the two planes.
        if ( checkQuaderStrutIntersection( this->brl, this->trl, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->brr, this->trr, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->bfl, this->tfl, spherePos, radius ) ||
             checkQuaderStrutIntersection( this->bfr, this->tfr, spherePos, radius ) )
        {
            return true;
        }
    }

    // Last case would be if the sphere contains any of the edge points of the Quader.
    {
        double radiusSquared = ( radius * radius );

        if ( isPointInSphereSquared( this->brl, spherePos, radiusSquared ) )
        {
            return true;
        }
    }

    // No intersection I guess.
    return false;
}

inline bool Sphere::intersectLine( const rw::V3d& pos, const rw::V3d& dir, double& first, double& second ) const
{
    return intersectSphereWithLine( dir, pos, this->point, this->radius, first, second );
}

Frustum::Frustum(const rw::Matrix& matrix)
{
	planes[0] = SimplePlane(-matrix.right.z, -matrix.up.z, -matrix.at.z, -matrix.pos.z);
	planes[1] = SimplePlane(matrix.right.z - matrix.rightw, matrix.up.z - matrix.upw, matrix.at.z - matrix.atw, matrix.pos.z - matrix.posw);
	planes[2] = SimplePlane(-matrix.rightw - matrix.right.x, -matrix.upw - matrix.up.x, -matrix.atw - matrix.at.x, -matrix.posw - matrix.pos.x);
	planes[3] = SimplePlane(matrix.right.x - matrix.rightw, matrix.up.x - matrix.upw, matrix.at.x - matrix.atw, matrix.pos.x - matrix.posw);
	planes[4] = SimplePlane(matrix.right.y - matrix.rightw, matrix.up.y - matrix.upw, matrix.at.y - matrix.atw, matrix.pos.y - matrix.posw);
	planes[5] = SimplePlane(-matrix.rightw - matrix.right.y, -matrix.upw - matrix.up.y, -matrix.atw - matrix.at.y, -matrix.posw - matrix.pos.y);

	planes[0].normalize();
	planes[1].normalize();
	planes[2].normalize();
	planes[3].normalize();
	planes[4].normalize();
	planes[5].normalize();

	createCorners();
}

static rw::V3d IntersectionPoint(const SimplePlane& a, const SimplePlane& b, const SimplePlane& c)
{
	rw::V3d v1, v2, v3;
	rw::V3d cross;

	float f;

	cross = rw::cross(b.normal, c.normal);

	f = rw::dot(a.normal, cross);
	f *= -1.0f;

	v1 = rw::scale(cross, a.dist);

	cross = rw::cross(c.normal, a.normal);
	v2 = rw::scale(cross, b.dist);

	cross = rw::cross(a.normal, b.normal);
	v3 = rw::scale(cross, c.dist);

	return rw::V3d(
		(v1.x + v2.x + v3.x) / f,
		(v1.y + v2.y + v3.y) / f,
		(v1.z + v2.z + v3.z) / f
	);
}

void Frustum::createCorners()
{
	corners[0] = IntersectionPoint(planes[0], planes[2], planes[4]);
	corners[1] = IntersectionPoint(planes[0], planes[3], planes[4]);
	corners[2] = IntersectionPoint(planes[0], planes[3], planes[5]);
	corners[3] = IntersectionPoint(planes[0], planes[2], planes[5]);
	corners[4] = IntersectionPoint(planes[1], planes[2], planes[4]);
	corners[5] = IntersectionPoint(planes[1], planes[3], planes[4]);
	corners[6] = IntersectionPoint(planes[1], planes[3], planes[5]);
	corners[7] = IntersectionPoint(planes[1], planes[2], planes[5]);
}

bool Frustum::isPointInside(const rw::V3d& point) const
{
	for (size_t i : irange<size_t>(6))
	{
		if ((rw::dot(point, planes[i].normal) + planes[i].dist) <= 0)
		{
			return false;
		}
	}

	return true;
}

bool Frustum::intersectWith(const Sphere& right) const
{
	for (size_t i : irange<size_t>(6))
	{
		if ((rw::dot(planes[i].normal, right.point) + planes[i].dist) > right.radius)
		{
			return false;
		}
	}

	return true;
}

bool Frustum::intersectWith(const Quader& right) const
{
	for (size_t i : irange<size_t>(6))
	{
		rw::V3d posVert;
		rw::V3d negVert;

		if (planes[i].normal.x >= 0)
		{
			posVert.x = right.tfr.x;
			negVert.x = right.brl.x;
		}
		else
		{
			posVert.x = right.brl.x;
			negVert.x = right.tfr.x;
		}

		if (planes[i].normal.y >= 0)
		{
			posVert.y = right.tfr.y;
			negVert.y = right.brl.y;
		}
		else
		{
			posVert.y = right.brl.y;
			negVert.y = right.tfr.y;
		}

		if (planes[i].normal.z >= 0)
		{
			posVert.z = right.tfr.z;
			negVert.z = right.brl.z;
		}
		else
		{
			posVert.z = right.brl.z;
			negVert.z = right.tfr.z;
		}

		float distance = rw::dot(planes[i].normal, negVert) + planes[i].dist;

		if (distance > 0.0f)
		{
			return false;
		}
	}

	return true;
}

// A cool testing thing, meow.
void Tests( void )
{
    // Test plane equations.
    {
        Plane slipperyPlane_frontSunRight;
        slipperyPlane_frontSunRight.pos = rw::V3d( -1, -1, -1 );
        slipperyPlane_frontSunRight.a = rw::V3d( 2, 0, 1 );
        slipperyPlane_frontSunRight.b = rw::V3d( 0, 2, 1 );

        Plane slipperyPlane_invFrontSunRight;
        slipperyPlane_invFrontSunRight.pos = rw::V3d( -1, -1, 1 );
        slipperyPlane_invFrontSunRight.a = rw::V3d( 2, 0, -1 );
        slipperyPlane_invFrontSunRight.b = rw::V3d( 0, 2, -1 );

        assert( slipperyPlane_frontSunRight.intersectWith( slipperyPlane_invFrontSunRight ) == true );
        assert( slipperyPlane_invFrontSunRight.intersectWith( slipperyPlane_frontSunRight ) == true );

        Plane floating_parallel_primary;
        floating_parallel_primary.pos = rw::V3d( 0, 0, -1 );
        floating_parallel_primary.a = rw::V3d( 1, 0, 0 );
        floating_parallel_primary.b = rw::V3d( 0, 1, 0 );

        Plane floating_parallel_secondary;
        floating_parallel_secondary.pos = rw::V3d( 0, 0, 1 );
        floating_parallel_secondary.a = rw::V3d( 1, 0, 0 );
        floating_parallel_secondary.b = rw::V3d( 0, 1, 0 );

        assert( floating_parallel_primary.intersectWith( floating_parallel_secondary ) == false );
        assert( floating_parallel_secondary.intersectWith( floating_parallel_primary ) == false );

        Plane plane_to_left;
        plane_to_left.pos = rw::V3d( -2, 0, 0 );
        plane_to_left.a = rw::V3d( 1, 0, 0 );
        plane_to_left.b = rw::V3d( 0, 1, 0 );

        Plane something_vertical;
        something_vertical.pos = rw::V3d( 0, 0, 0 );
        something_vertical.a = rw::V3d( 0, 0, 3 );
        something_vertical.b = rw::V3d( 0, 1, 0 );

        assert( plane_to_left.intersectWith( something_vertical ) == false );
        assert( something_vertical.intersectWith( plane_to_left ) == false );

        assert( something_vertical.intersectWith( slipperyPlane_frontSunRight ) == true );
        assert( something_vertical.intersectWith( slipperyPlane_invFrontSunRight ) == true );

        // Check some specially designed planes for intersection.
        // I dare you to find a plane equation that cannot be solved by my code.
        
        Plane tigerPlane;
        tigerPlane.pos = rw::V3d( 120, 120, 300 );
        tigerPlane.a = rw::V3d( 100, 0, 0 );
        tigerPlane.b = rw::V3d( 0, 100, 3 );

        Plane intruderPlane;
        intruderPlane.pos = rw::V3d( 200, 140, 190 );
        intruderPlane.a = rw::V3d( 1, 0, 0 );
        intruderPlane.b = rw::V3d( 0, 0, 1000 );

        assert( tigerPlane.intersectWith( intruderPlane ) == true );
        assert( intruderPlane.intersectWith( tigerPlane ) == true );

        assert( tigerPlane.intersectWith( floating_parallel_primary ) == false );
        assert( tigerPlane.intersectWith( floating_parallel_secondary ) == false );

        assert( floating_parallel_primary.intersectWith( tigerPlane ) == false );
        assert( floating_parallel_primary.intersectWith( intruderPlane ) == false );
    }

    // Test some triangle planes, haha.
    // This is highly complicated stuff m8.
    {
        TrianglePlane tni;
        tni.pos = rw::V3d( 0, 0, 0 );
        tni.a = rw::V3d( 0, 0, 5 );
        tni.b = rw::V3d( 0, 5, 0 );

        TrianglePlane intrude;
        intrude.pos = rw::V3d( -5, 3, 3 );
        intrude.a = rw::V3d( 10, 0, 0 );
        intrude.b = rw::V3d( 0, 0, 10 );

        assert( tni.intersectWith( intrude ) == false );
        assert( intrude.intersectWith( tni ) == false );

        Plane tni_inv;
        tni_inv.pos = tni.pos;
        tni_inv.a = tni.a;
        tni_inv.b = tni.b;

        Plane intrude_inv;
        intrude_inv.pos = intrude.pos;
        intrude_inv.a = intrude.a;
        intrude_inv.b = intrude.b;

        assert( tni_inv.intersectWith( intrude_inv ) == true );
        assert( intrude_inv.intersectWith( tni_inv ) == true );

        TrianglePlane the_obvious;
        the_obvious.pos = rw::V3d( -4, 1, 1 );
        the_obvious.a = rw::V3d( 100, 0, 0 );
        the_obvious.b = rw::V3d( 0, 20, 0 );

        assert( tni.intersectWith( the_obvious ) == true );
        assert( the_obvious.intersectWith( tni ) == true );
    }

    // Build some quader and see logically if things make sense.
    Quader theQuaderOfTruth(
        // *bottom
        rw::V3d( -1, -1, -1 ),
        rw::V3d(  1, -1, -1 ),
        rw::V3d( -1,  1, -1 ),
        rw::V3d(  1,  1, -1 ),
        // *top
        rw::V3d( -1, -1,  1 ),
        rw::V3d(  1, -1,  1 ),
        rw::V3d( -1,  1,  1 ),
        rw::V3d(  1,  1,  1 )
    );

    // Those points should really stay outside :)
    assert( theQuaderOfTruth.isPointInside( rw::V3d( -1.1f, 0, 0 ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 1.1f, 0 ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 0, 1.1f ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, -1.1f, 0 ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 0, -1.1f ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 0, 1.1f ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 2, 2, 2 ) ) == false );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( -2, -2, -2 ) ) == false );

    // Now lets see if points can be inside the quader.
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 0, 0 ) ) == true );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0.5f, 0.5f, 0.5f ) ) == true );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( -0.5f, 0.5f, -0.4f ) ) == true );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 1, 0, 0 ) ) == true );
    assert( theQuaderOfTruth.isPointInside( rw::V3d( 0, 0.2f, -1 ) ) == true );

    // Test some nice other quaders, meow.
    {
        Quader quaderInside(
            // *bottom
            rw::V3d( -1, -0.5f, -0.5f ),
            rw::V3d(  1, -0.5f, -0.5f ),
            rw::V3d( -1,  1, -1 ),
            rw::V3d(  1,  1, -1 ),
            // *top
            rw::V3d( -1, -1,  1 ),
            rw::V3d(  1, -1,  1 ),
            rw::V3d( -1,  1,  1 ),
            rw::V3d(  1,  1,  1 )
        );

        assert( theQuaderOfTruth.intersectWith( quaderInside ) == true );
        assert( quaderInside.intersectWith( theQuaderOfTruth ) == true );

        const rw::V3d offPos( 2.2f, 0, 0 );

        Quader irrelevant(
            // *bottom
            rw::add( theQuaderOfTruth.brl, offPos ),
            rw::add( theQuaderOfTruth.brr, offPos ),
            rw::add( theQuaderOfTruth.bfl, offPos ),
            rw::add( theQuaderOfTruth.bfr, offPos ),
            // *top
            rw::add( theQuaderOfTruth.trl, offPos ),
            rw::add( theQuaderOfTruth.trr, offPos ),
            rw::add( theQuaderOfTruth.tfl, offPos ),
            rw::add( theQuaderOfTruth.tfr, offPos )
        );

        assert( theQuaderOfTruth.intersectWith( irrelevant ) == false );
        assert( irrelevant.intersectWith( theQuaderOfTruth ) == false );

        Quader sideIntrude(
            // *bottom
            rw::V3d( -20, -0.75f, -0.8f ),
            rw::V3d(  20, -0.75f, -0.8f ),
            rw::V3d( -20,  0.75f, -0.8f ),
            rw::V3d(  20,  0.75f, -0.8f ),
            // *top
            rw::V3d( -20, -0.75f, 0.8f ),
            rw::V3d(  20, -0.75f, 0.8f ),
            rw::V3d( -20,  0.75f, 0.8f ),
            rw::V3d(  20,  0.75f, 0.8f )
        );

        assert( sideIntrude.intersectWith( theQuaderOfTruth ) == true );
        assert( theQuaderOfTruth.intersectWith( sideIntrude ) == true );
    }

    // Get some awkward spheres and check things out.
    {
        Sphere leftPlaneTouch;
        leftPlaneTouch.point = rw::V3d( -1.5f, 0, 0 );
        leftPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( leftPlaneTouch ) == false );

        Sphere rearPlaneTouch;
        rearPlaneTouch.point = rw::V3d( 0, -1.5f, 0 );
        rearPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( rearPlaneTouch ) == false );

        Sphere rightPlaneTouch;
        rightPlaneTouch.point = rw::V3d( 1.5f, 0, 0 );
        rightPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( rightPlaneTouch ) == false );

        Sphere frontPlaneTouch;
        frontPlaneTouch.point = rw::V3d( 0, 1.5f, 0 );
        frontPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( frontPlaneTouch ) == false );

        Sphere topPlaneTouch;
        topPlaneTouch.point = rw::V3d( 0, 0, 1.5f );
        topPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( topPlaneTouch ) == false );

        Sphere bottomPlaneTouch;
        bottomPlaneTouch.point = rw::V3d( 0, 0, -1.5f );
        bottomPlaneTouch.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( bottomPlaneTouch ) == false );

        Sphere insideSphere;
        insideSphere.point = rw::V3d( 0.2f, -0.6f, 0.3f );
        insideSphere.radius = 1;

        assert( theQuaderOfTruth.intersectWith( insideSphere ) == true );

        Sphere floatingInSpace;
        floatingInSpace.point = rw::V3d( 1.5f, 1.5f, 1.5f );
        floatingInSpace.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( floatingInSpace ) == false );

        Sphere topPlaneStrutRearIntersect;
        topPlaneStrutRearIntersect.point = rw::V3d( 0, -1.1f, 1.1f );
        topPlaneStrutRearIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( topPlaneStrutRearIntersect ) == true );

        Sphere topPlaneStrutRightIntersect;
        topPlaneStrutRightIntersect.point = rw::V3d( 1.1f, 0, 1.1f );
        topPlaneStrutRightIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( topPlaneStrutRightIntersect ) == true );

        Sphere topPlaneStrutFrontIntersect;
        topPlaneStrutFrontIntersect.point = rw::V3d( 0, 1.1f, 1.1f );
        topPlaneStrutFrontIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( topPlaneStrutFrontIntersect ) == true );

        Sphere topPlaneStrutLeftIntersect;
        topPlaneStrutLeftIntersect.point = rw::V3d( -1.1f, 0, 1.1f );
        topPlaneStrutLeftIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( topPlaneStrutLeftIntersect ) == true );

        Sphere bottomPlaneStrutRearIntersect;
        bottomPlaneStrutRearIntersect.point = rw::V3d( 0, -1.1f, -1.1f );
        bottomPlaneStrutRearIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( bottomPlaneStrutRearIntersect ) == true );

        Sphere bottomPlaneStrutRightIntersect;
        bottomPlaneStrutRightIntersect.point = rw::V3d( -1.1f, 0, -1.1f );
        bottomPlaneStrutRightIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( bottomPlaneStrutRightIntersect ) == true );

        Sphere bottomPlaneStrutFrontIntersect;
        bottomPlaneStrutFrontIntersect.point = rw::V3d( 0, 1.1f, -1.1f );
        bottomPlaneStrutFrontIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( bottomPlaneStrutFrontIntersect ) == true );

        Sphere bottomPlaneStrutLeftIntersect;
        bottomPlaneStrutLeftIntersect.point = rw::V3d( -1.1f, 0, -1.1f );
        bottomPlaneStrutLeftIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( bottomPlaneStrutLeftIntersect ) == true );

        Sphere connectStrutRearLeftIntersect;
        connectStrutRearLeftIntersect.point = rw::V3d( -1.1f, -1.1f, 0 );
        connectStrutRearLeftIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( connectStrutRearLeftIntersect ) == true );

        Sphere connectStrutRearRightIntersect;
        connectStrutRearRightIntersect.point = rw::V3d( 1.1f, -1.1f, 0 );
        connectStrutRearRightIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( connectStrutRearRightIntersect ) == true );

        Sphere connectStrutFrontLeftIntersect;
        connectStrutFrontLeftIntersect.point = rw::V3d( -1.1f, 1.1f, 0 );
        connectStrutFrontLeftIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( connectStrutFrontLeftIntersect ) == true );

        Sphere connectStrutFrontRightIntersect;
        connectStrutFrontRightIntersect.point = rw::V3d( 1.1f, 1.1f, 0 );
        connectStrutFrontRightIntersect.radius = 0.5;

        assert( theQuaderOfTruth.intersectWith( connectStrutFrontRightIntersect ) == true );

        Sphere bigEnclosingSphere;
        bigEnclosingSphere.point = rw::V3d( -1.5f, -1.5f, -1.5f );
        bigEnclosingSphere.radius = 9999;

        assert( theQuaderOfTruth.intersectWith( bigEnclosingSphere ) == true );
    }

    // Yay, we succeeded.
}

}
}