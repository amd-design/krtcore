#pragma once

// Common helpers because aap refuses to put good coding practice of C++ into his library.

namespace rw
{

template <typename cbType>
inline void clumpForAllAtomics( rw::Clump *clump, cbType& cb )
{
    FORLIST(lnk, clump->atomics)
        cb( Atomic::fromClump( lnk ) );
}

template <typename cbType>
inline void clumpForAllLights( rw::Clump *clump, cbType& cb )
{
    FORLIST(lnk, clump->lights)
        cb( Light::fromClump( lnk ) );
}

template <typename cbType>
inline void clumpForAllCameras( rw::Clump *clump, cbType& cb )
{
    FORLIST(lnk, clump->cameras)
        cb( Camera::fromClump( lnk ) );
}

};