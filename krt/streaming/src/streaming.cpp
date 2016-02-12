#include "StdInc.h"
#include "streaming.h"

namespace Streaming
{

StreamMan::StreamMan( void )
{

}

StreamMan::~StreamMan( void )
{

}

bool StreamMan::Request( ident_t id )
{
    return false;
}

void StreamMan::CancelRequest( ident_t id )
{

}

StreamMan::eResourceStatus StreamMan::GetResourceStatus( ident_t id ) const
{
    return eResourceStatus::UNLOADED;
}

void StreamMan::GetStatistics( StreamingStats& statsOut ) const
{
    // TODO.
}

bool StreamMan::RegisterResourceType( ident_t base, ident_t range, StreamingTypeInterface *intf )
{
    return false;
}

bool StreamMan::UnregisterResourceType( ident_t base )
{
    return false;
}

};