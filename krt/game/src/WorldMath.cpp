#include "StdInc.h"
#include "WorldMath.h"

namespace krt
{
namespace math
{

inline float ivec( const rw::V3d v, int idx )
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

// SPECIAL PLANE EQUATION SOLUTIONS.
// GTFO IF YOU DONT KNOW WHAT YOU ARE DOING M8.
typedef double p_resolve_t;

// Helper math.
MATH_INLINE p_resolve_t block_trans( const rw::V3d& t, const rw::V3d& b, int idxPrim, int idxSec )
{
    return ivec( t, idxPrim ) / ivec( b, idxPrim ) - ivec( t, idxPrim ) / ivec( b, idxSec );
}

// The most general solution, kinda. Could also say most complicated.
inline bool plane_solution_fetcht_type_a(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    p_resolve_t b, p_resolve_t& result
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
        p_resolve_t t =
            (
                b * ( block_trans( z, x, 1, 2 ) / mod_point_ux12 - block_trans( z, x, 3, 2 ) / mod_point_ux32 )
                +
                (
                    ( block_trans( o, x, 1, 2 ) - block_trans( p, x, 1, 2 ) ) / mod_point_ux12
                    -
                    ( block_trans( o, x, 3, 2 ) - block_trans( p, x, 3, 2 ) ) / mod_point_ux32
                )
            )
            / mod_point_yx12_yx32_diff;

        result = t;
        return true;
    }

    p_resolve_t mod_point_yx13 = block_trans( y, x, 1, 3 );
    
    p_resolve_t mod_point_yx12_yx13_diff = ( mod_point_yx12 - mod_point_yx13 ); // MUST NOT BE ZERO

    p_resolve_t mod_point_ux13 = block_trans( u, x, 1, 3 ); // MUST NOT BE ZERO

    if ( mod_point_yx12_yx13_diff != 0 && mod_point_ux12 != 0 && mod_point_ux13 != 0 )
    {
        p_resolve_t t =
            (
                b * ( block_trans( z, x, 1, 2 ) / mod_point_ux12 - block_trans( z, x, 1, 3 ) / mod_point_ux13 )
                +
                (
                    ( block_trans( o, x, 1, 2 ) - block_trans( p, x, 1, 2 ) ) / mod_point_ux12
                    -
                    ( block_trans( o, x, 1, 3 ) - block_trans( p, x, 1, 3 ) ) / mod_point_ux13
                )
            )
            / mod_point_yx12_yx13_diff;

        result = t;
        return true;
    }

    p_resolve_t mod_point_yx32_yx13_diff = ( mod_point_yx32 - mod_point_yx13 );

    if ( mod_point_yx32_yx13_diff != 0 && mod_point_ux32 != 0 && mod_point_ux13 != 0 )
    {
        p_resolve_t t =
            (
                b * ( block_trans( z, x, 3, 2 ) / mod_point_ux32 - block_trans( z, x, 1, 3 ) / mod_point_ux13 )
                +
                (
                    ( block_trans( o, x, 3, 2 ) - block_trans( p, x, 3, 2 ) ) / mod_point_ux32
                    -
                    ( block_trans( o, x, 1, 3 ) - block_trans( p, x, 1, 3 ) ) / mod_point_ux13
                )
            )
            / mod_point_yx32_yx13_diff;

        result = t;
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
    p_resolve_t b, p_resolve_t& result,
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
        if ( ivec( u, datIndex != 0 ) )
        {
            // We got a solution, probably.
            p_resolve_t mod_point_ux = block_trans( u, x, prim_divide, sec_divide );

            if ( mod_point_ux != 0 )
            {
                p_resolve_t major_divisor = ( ivec( y, datIndex ) / ivec( u, datIndex ) - block_trans( y, x, prim_divide, sec_divide ) );

                if ( major_divisor != 0 )
                {
                    // Alright, we should have a solution here.
                    p_resolve_t t =
                        (
                            b * ( ivec( z, datIndex ) / ivec( u, datIndex ) - block_trans( z, x, prim_divide, sec_divide ) )
                            +
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

                    result = t;
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
                        p_resolve_t t =
                            (
                                b * ( ivec( z, datIndex ) / ivec( u, datIndex ) - block_trans( z, x, prim_divide, sec_divide ) / modulator )
                                +
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

                        result = t;
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
                    p_resolve_t t =
                        (
                            b * block_trans( z, x, prim_divide, sec_divide )
                            +
                            (
                                ( ivec( o, prim_divide ) - ivec( p, prim_divide ) ) / ivec( x, prim_divide )
                                -
                                ( ivec( o, sec_divide ) - ivec( p, sec_divide ) ) / ivec( x, sec_divide )
                            )
                        ) / divisor;

                    result = t;
                    return true;
                }
            }
        }
    }

    // And even more...

    if ( ivec( y, datIndex ) != 0 )
    {
        // Pretty cool little solution right here.
        p_resolve_t t = ( b * ivec( z, datIndex ) + ivec( o, datIndex ) - ivec( p, datIndex ) ) / ivec( y, datIndex );

        result = t;
        return true;
    }

    // No result.
    return false;
}

// The cutest solution yet.
inline bool plane_solution_fetcht_type_c(
    const rw::V3d& u, const rw::V3d& z, const rw::V3d& o,
    const rw::V3d& x, const rw::V3d& y, const rw::V3d& p,
    p_resolve_t b, p_resolve_t& result,
    int prim_idx, int sec_idx
)
{
    assert( ivec( x, prim_idx == 0 ) && ivec( x, sec_idx ) == 0 );

    // It ain't that hard even, meow.
    p_resolve_t divisor = block_trans( y, u, prim_idx, sec_idx );

    if ( divisor )
    {
        p_resolve_t t =
            (
                b * block_trans( z, u, prim_idx, sec_idx )
                +
                (
                    ( ivec( o, prim_idx ) - ivec( p, prim_idx ) ) / ivec( u, prim_idx )
                    -
                    ( ivec( o, sec_idx ) - ivec( p, sec_idx ) ) / ivec( u, sec_idx )
                )
            ) / divisor;

        result = t;
        return true;
    }

    return false;
}

bool Plane::intersectWith( const Plane& right ) const
{
    // Try some shitty math.
    // I hope I got all cases of plane-plane intersection figured out.

    // Legend: x is vector a of this, s is scalar
    //         y is vector b of this, t is scalar
    //         u is vector a of right, a is scalar
    //         z is vector b of right, b is scalar
    //         p is position of this
    //         o is position of right

    rw::V3d x = this->a;
    rw::V3d y = this->b;
    rw::V3d u = right.a;
    rw::V3d z = right.b;
    rw::V3d p = this->pos;
    rw::V3d o = right.pos;

    bool hasIntersection = false;

    // Check all possible cases.
    // We first do get the value of t depending on b.
    // Then we resolve s depending on t, resolve a depending on b.
    p_resolve_t t0, t1;
    bool hasSolution = false;   // true if we found a solution for t based on b.
    {
        p_resolve_t b = 1;

        // Specialized math.
        if ( ivec( x, 1 ) != 0 && ivec( x, 2 ) != 0 && ivec( x, 3 ) != 0 )
        {
            if ( plane_solution_fetcht_type_a( u, z, o, x, y, p, 0, t0 ) &&
                 plane_solution_fetcht_type_a( u, z, o, x, y, p, 1, t1 ) )
            {
                hasSolution = true;
            }
        }

        if ( !hasSolution )
        {
            // Always search further if no solution has been found yet.

            if ( ivec( x, 1 ) == 0 )
            {
                if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, 0, t0, 1 ) &&
                     plane_solution_fetcht_type_b( u, z, o, x, y, p, 1, t1, 1 ) )
                {
                    hasSolution = true;
                }
                // TODO: check special solution.
            }
            
            if ( !hasSolution )
            {
                if ( ivec( x, 2 ) == 0 )
                {
                    if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, 0, t0, 2 ) &&
                         plane_solution_fetcht_type_b( u, z, o, x, y, p, 1, t1, 2 ) )
                    {
                        hasSolution = true;
                    }
                    // TODO: check special solution.
                }
            
                if ( !hasSolution )
                {
                    if ( ivec( x, 3 ) == 0 )
                    {
                        if ( plane_solution_fetcht_type_b( u, z, o, x, y, p, 0, t0, 3 ) &&
                             plane_solution_fetcht_type_b( u, z, o, x, y, p, 1, t1, 3 ) )
                        {
                            hasSolution = true;
                        }
                        // TODO: check special solution.
                    }

                    // Now check for solutions if two coordinates are zero.
                    if ( ivec( x, 1 ) == 0 && ivec( x, 2 ) == 0 )
                    {
                        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, 0, t0, 1, 2 ) &&
                             plane_solution_fetcht_type_c( u, z, o, x, y, p, 1, t1, 1, 2 ) )
                        {
                            hasSolution = true;
                        }
                    }
                    else if ( ivec( x, 2 ) == 0 && ivec( x, 3 ) == 0 )
                    {
                        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, 0, t0, 2, 3 ) &&
                             plane_solution_fetcht_type_c( u, z, o, x, y, p, 1, t1, 2, 3 ) )
                        {
                            hasSolution = true;
                        }
                    }
                    else if ( ivec( x, 1 ) == 0 && ivec( x, 3 ) == 0 )
                    {
                        if ( plane_solution_fetcht_type_c( u, z, o, x, y, p, 0, t0, 1, 3 ) &&
                             plane_solution_fetcht_type_c( u, z, o, x, y, p, 1, t1, 1, 3 ) )
                        {
                            hasSolution = true;
                        }
                    }

                    // There are no more solutions, meow.
                }
            }
        }
    }

    // Do we have a generic intersection based on dependency by b ?
    if ( hasSolution )
    {
        // Get the s0 and s1 coodinates for the t0 and t1 points.

    }

    // No intersection :(
    return false;
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

inline Quader::Quader(
    rw::V3d brl, rw::V3d brr, rw::V3d bfl, rw::V3d bfr,
    rw::V3d trl, rw::V3d trr, rw::V3d tfl, rw::V3d tfr
) : brl( brl ), brr( brr ), bfl( bfl ), bfr( bfr ),
    trl( trl ), trr( trr ), tfl( tfl ), tfr( tfr )
{
    // Verify that we really make up a quader!
    assert( verifyPlanePointConfiguration( brl, brr, trl, trr ) == true );  // rear plane
    assert( verifyPlanePointConfiguration( bfl, brl, tfl, trl ) == true );  // left plane
    assert( verifyPlanePointConfiguration( bfr, bfl, tfr, tfl ) == true );  // front plane
    assert( verifyPlanePointConfiguration( brr, bfr, trr, tfr ) == true );  // right plane
    assert( verifyPlanePointConfiguration( brr, brl, bfr, bfl ) == true );  // bottom plane
    assert( verifyPlanePointConfiguration( trr, trl, tfr, tfl ) == true );  // top plane

    // I trust the programmer that he knows how to select 8 valid points for a Quader.
    // If we really cannot into that, then fml.
    // The above logic should fault if there is a problem.
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
    // Should be a simple math thing, so do not overuse this function.

    rw::Matrix inv_bottomMat, inv_topMat;

    bool couldSetup = setupQuaderMatrices( *this, inv_bottomMat, inv_topMat );

    if ( !couldSetup )
        return false;

    return isPointInInclusionSpace( inv_bottomMat, inv_topMat, point, true );
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

bool Quader::intersectWith( const Quader& right ) const
{
    // A Quader is a 3 dimensional space that can be mapped between 8 points.
    // It looks like a deformed cube.
    {
        rw::Matrix inv_bottomMat, inv_topMat;

        bool couldSetup = setupQuaderMatrices( *this, inv_bottomMat, inv_topMat );

        if ( !couldSetup )
            return false;
        
        // Check points of the right quader in the local system of this quader.
        if ( isQuaderConfigurationIntersectingLocalSpace( inv_bottomMat, inv_topMat, right, false ) )
        {
            return true;
        }
    }

    {
        rw::Matrix inv_bottomMat, inv_topMat;

        bool couldSetup = setupQuaderMatrices( right, inv_bottomMat, inv_topMat );

        if ( !couldSetup )
            return false;

        // Check points of this quader in the local space of the right quader.
        if ( isQuaderConfigurationIntersectingLocalSpace( inv_bottomMat, inv_topMat, *this, true ) )
        {
            return true;
        }
    }

    // Dunno. We do not appear to be intersecting.
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

inline bool calculateSpherePlaneAdjacencyIntersect(
    rw::V3d bl, rw::V3d br, rw::V3d tl, rw::V3d tr,
    rw::V3d spherePos, float radius,
    bool& result
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
    rw::Matrix inv_planeSpace_bottom, inv_planeSpace_top;
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

    // Get the distance of the sphere center to the plane.
    rw::V3d localSpace_bottom = inv_planeSpace_bottom.transPoint( spherePos );
    rw::V3d localSpace_top = inv_planeSpace_top.transPoint( spherePos );

    // That should be same if we have a valid Quader.
    assert( localSpace_bottom.y == localSpace_top.y );

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

    rw::Matrix inv_bottomMat, inv_topMat;

    bool couldSetup = setupQuaderMatrices( *this, inv_bottomMat, inv_topMat );

    if ( !couldSetup )
        return false;

    // It is important to determine where exactly the sphere is.
    const rw::V3d spherePos = right.point;

    rw::V3d localSpace_bottom = ((rw::Matrix&)inv_bottomMat).transPoint( spherePos );
    rw::V3d localSpace_top = ((rw::Matrix&)inv_topMat).transPoint( spherePos );

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

        if ( calculateSpherePlaneAdjacencyIntersect(    //rear
                 brl, brr, trl, trr,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the rear plane.
            return result;
        }

        if ( calculateSpherePlaneAdjacencyIntersect(    //left
                 bfl, brl, tfl, trl,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the left plane.
            return result;
        }

        if ( calculateSpherePlaneAdjacencyIntersect(    //front
                 bfr, bfl, tfr, tfl,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the front plane.
            return result;
        }

        if ( calculateSpherePlaneAdjacencyIntersect(    //right
                 brr, bfr, trr, tfr,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the right plane.
            return result;
        }
             
        if ( calculateSpherePlaneAdjacencyIntersect(    //top
                 trl, trr, tfl, tfr,
                 spherePos, radius,
                 result
             ) )
        {
            // We could intersect the top plane.
            return result;
        }
             
        if ( calculateSpherePlaneAdjacencyIntersect(    //bottom
                 bfl, bfr, brl, brr,
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

// A cool testing thing, meow.
void Tests( void )
{
    // Test plane equations.
    {
        Plane slipperyPlane_frontSunRight;
        slipperyPlane_frontSunRight.pos = rw::V3d( -1, -1, -1 );
        slipperyPlane_frontSunRight.a = rw::V3d( 2, 0, 2 );
        slipperyPlane_frontSunRight.b = rw::V3d( 0, 2, 2 );

        Plane slipperyPlane_invFrontSunRight;
        slipperyPlane_invFrontSunRight.pos = rw::V3d( -1, -1, 1 );
        slipperyPlane_invFrontSunRight.a = rw::V3d( 2, 0, -2 );
        slipperyPlane_invFrontSunRight.b = rw::V3d( 0, 2, -2 );

#if 0
        assert( slipperyPlane_frontSunRight.intersectWith( slipperyPlane_invFrontSunRight ) == true );
        assert( slipperyPlane_invFrontSunRight.intersectWith( slipperyPlane_frontSunRight ) == true );
#endif
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
            rw::V3d(  1, -0.5, -0.5 ),
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