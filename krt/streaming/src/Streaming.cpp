#include "StdInc.h"
#include "Streaming.h"

#include <windows.h>

#define STREAMING_DEFAULT_MAX_MEMORY            10000000 //meow

// NOTE that this is a very new system that needs ironing out bugs.
// So report any issue that you can find!

namespace krt
{
namespace streaming
{

template <typename lockType>
struct shared_lock_acquire
{
    inline shared_lock_acquire( lockType& theLock ) : theLock( theLock )
    {
        theLock.lock_shared();
    }

    inline ~shared_lock_acquire( void )
    {
        theLock.unlock_shared();
    }

    lockType& theLock;
};

template <typename lockType>
struct exclusive_lock_acquire
{
    inline exclusive_lock_acquire( lockType& theLock ) : theLock( theLock )
    {
        theLock.lock();
    }

    inline ~exclusive_lock_acquire( void )
    {
        theLock.unlock();
    }

    lockType& theLock;
};

void StreamMan::StreamingChannelRuntime( void *ud )
{
    StreamMan *manager = NULL;
    Channel *channel = NULL;
    {
        const channel_param *params = (const channel_param*)ud;

        manager = params->manager;
        channel = params->channel;

        // We must clean up after ourselves.
        delete params;
    }

    while ( true )
    {
        // Wait for requests to become available.
        {
            HANDLE waitHandles[] =
            {
                channel->terminationEvent,
                channel->semRequestCount
            };

            DWORD waitResult = WaitForMultipleObjects( _countof(waitHandles), waitHandles, FALSE, INFINITE );

            if ( waitResult == WAIT_OBJECT_0 )
            {
                // Our thread termination event was signaled.
                break;
            }
        }

        // Take a request and fulfill it!
        bool hasRequest = false;
        Channel::request_t request;
        {
            exclusive_lock_acquire <std::shared_timed_mutex> ctxFetchRequest( channel->channelLock );

            if ( channel->requests.empty() == false )
            {
                request = channel->requests.front();

                channel->requests.pop_front();

                hasRequest = true;
            }
        }

        // We have to check whether this request makes any sense.
        // Do that in a minimal verification phase.
        Resource *resToLoad = NULL;

        if ( hasRequest )
        {
            exclusive_lock_acquire <std::shared_timed_mutex> ctxResLoadAcquire( manager->lockResourceContest );

            Resource *wantedResource = manager->GetResourceAtID( request.resID );

            if ( wantedResource )
            {
                if ( wantedResource->status == eResourceStatus::UNLOADED )
                {
                    // We will start by buffering things.
                    wantedResource->status = eResourceStatus::BUFFERING;

                    // We can load currently not loaded things, so proceed.
                    resToLoad = wantedResource;
                }
            }
        }

        if ( resToLoad )
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxLoadResource( manager->lockResourceContest );
            
            // Get the resource that we are meant to load.
            reg_streaming_type *typeInfo = manager->GetStreamingTypeAtID( request.resID );

            if ( typeInfo )
            {
                StreamingTypeInterface *streamingType = typeInfo->manager;

                // Ensure that we have enough space in our loading buffer to store the resource.
                size_t resourceSize = resToLoad->resourceSize;
                {
                    if ( channel->dataBuffer.size() < resourceSize )
                    {
                        channel->dataBuffer.resize( resourceSize );
                    }
                }

                void *dataBuffer = channel->dataBuffer.data();

                // Load this resource.
                ident_t localID = ( request.resID - typeInfo->base );

                resToLoad->location->fetchData( dataBuffer );

                // Transition state from BUFFERING to LOADING.
                resToLoad->status = eResourceStatus::LOADING;

                // Give this data to the runtime.
                streamingType->LoadResource( localID, dataBuffer, resourceSize );

                // We are now loaded!
                resToLoad->status = eResourceStatus::LOADED;

                manager->totalStreamingMemoryUsage += resourceSize;
            }
        }

        // Need to clear activity flag.
        if ( hasRequest )
        {
            exclusive_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( channel->channelLock );

            // We are not active anymore if all requests are fulfilled.
            if ( channel->requests.empty() == true )
            {
                channel->isActive = false;

                // We can unlock any waiting people.
                channel->condIsActive.notify_all();
            }
        }
    }

    return;
}

StreamMan::Channel::Channel( StreamMan *manager )
{
    this->manager = manager;

    channel_param *params = new channel_param;
    params->manager = manager;
    params->channel = this;

    // Semaphore request availability semaphore.
    this->semRequestCount = CreateSemaphoreW( NULL, 0, 9000, NULL );
    this->terminationEvent = CreateEventW( NULL, TRUE, FALSE, NULL );

    this->isActive = false;

    this->thread = std::thread([=] ()
    {
        StreamingChannelRuntime(params);
    });
}

StreamMan::Channel::~Channel( void )
{
    // Wait for thread termination.
    SetEvent(terminationEvent);

    this->thread.join();

    // Clean up management stuff.
    CloseHandle( this->semRequestCount );
    CloseHandle( this->terminationEvent );
}

StreamMan::StreamMan( unsigned int numChannels ) : totalStreamingMemoryUsage( 0 ), maxMemory( STREAMING_DEFAULT_MAX_MEMORY )
{
    // Initialize management variables.
    this->currentChannelID = 0;

    // Spawn channels for loading.
    for ( unsigned int n = 0; n < numChannels; n++ )
    {
        this->channels.push_back( std::make_unique<Channel>(this) );
    }
}

StreamMan::~StreamMan( void )
{
    // Clear all channels.
    {
        this->channels.clear();
    }
}

bool StreamMan::Request( ident_t id )
{
    // Send this resource loading request to an available Channel.
    // Channels take loading requests on their own threads and provide data to the engine.

    shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( this->lockResourceContest );

    size_t channelCount = this->channels.size();

    unsigned int selChannel = ( this->currentChannelID++ % channelCount );

    // Give this channel the request.
    {
        Channel *channel = this->channels[ selChannel ].get();

        exclusive_lock_acquire <std::shared_timed_mutex> ctxPushChannelRequest( channel->channelLock );

        assert( channel != NULL );

        Channel::request_t newRequest;
        newRequest.resID = id;

        channel->requests.push_back( std::move( newRequest ) );

        // We have to lock the channel as active.
        // This is required to properly wait until it has finished loading.
        channel->isActive = true;

        // Notify the channel that a new request is available!
        ReleaseSemaphore( channel->semRequestCount, 1, NULL );
    }

    return true;
}

bool StreamMan::CancelRequest( ident_t id )
{
    // TODO: allow requests to be cancelled.
    // does not have to work in all cases.

    return false;
}

void StreamMan::LoadingBarrier( void )
{
    // Wait until all requests have been processed by all channels.

    // Check all channels.
    for ( auto& curChannel : this->channels )
    {
        std::unique_lock <std::mutex> lockWaitActive ( curChannel->lockIsActive );

        curChannel->condIsActive.wait( lockWaitActive,
            [&]
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( curChannel->channelLock );

            return ( ! ( curChannel->requests.empty() == false || curChannel->isActive ) );
        });
    }
}

StreamMan::eResourceStatus StreamMan::GetResourceStatus( ident_t id ) const
{
    shared_lock_acquire <std::shared_timed_mutex> ctxResourceStateFetch( this->lockResourceContest );

    const Resource *theRes = this->GetConstResourceAtID( id );

    if ( theRes )
    {
        return theRes->status;
    }

    // Dunno :(
    return eResourceStatus::UNLOADED;
}

void StreamMan::GetStatistics( StreamingStats& statsOut ) const
{
    statsOut.maxMemory = this->maxMemory;
    statsOut.memoryInUse = this->totalStreamingMemoryUsage;
}

bool StreamMan::CheckTypeRegionConflict( ident_t base, ident_t range ) const
{
    identSlice_t identSector( base, range );

    for ( const reg_streaming_type& regType : this->types )
    {
        identSlice_t conflictRegion( regType.base, regType.range );

        identSlice_t::eIntersectionResult result =
            identSector.intersectWith( conflictRegion );

        // Basically, we only allow that the things are floating apart.
        bool isFloatingApart = identSlice_t::isFloatingIntersect( result );

        if ( !isFloatingApart )
        {
            return true;
        }
    }

    return false;
}

StreamMan::reg_streaming_type* StreamMan::GetStreamingTypeAtID( ident_t id )
{
    identSlice_t identSector( id, 1 );

    for ( reg_streaming_type& regType : this->types )
    {
        identSlice_t conflictRegion( regType.base, regType.range );

        identSlice_t::eIntersectionResult result =
            identSector.intersectWith( conflictRegion );

        // Basically, we only allow that the things are floating apart.
        bool isFloatingApart = identSlice_t::isFloatingIntersect( result );

        if ( !isFloatingApart )
        {
            return &regType;
        }
    }

    return NULL;
}

void StreamMan::ClearResourcesAtSlot( ident_t resID, ident_t range )
{
    // We clear all available resources at said slots.
    for ( ident_t off = 0; off < range; off++ )
    {
        ident_t curID = ( off + resID );

        // This is equivalent to unlinking it.
        this->UnlinkResourceNative( curID );
    }
}

bool StreamMan::RegisterResourceType( ident_t base, ident_t range, StreamingTypeInterface *intf )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxRegisterType( this->lockResourceContest );

    // Register an entirely new streaming type, just for your enjoyment!

    // Check whether the range would intersect with any already registered one.
    // We do not want that.
    bool isConflict = CheckTypeRegionConflict( base, range );

    if ( isConflict )
        return false;   // meow.

    reg_streaming_type typeInfo;
    typeInfo.manager = intf;
    typeInfo.base = base;
    typeInfo.range = range;

    this->types.push_back( std::move( typeInfo ) );

    return true;
}

bool StreamMan::UnregisterResourceType( ident_t base )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxUnregisterType( this->lockResourceContest );

    // We get the streaming type at the given offset and unregister it.
    reg_streaming_type *streamType = GetStreamingTypeAtID( base );

    if ( streamType == NULL )
        return false;

    // Clean up stuff.
    ClearResourcesAtSlot( streamType->base, streamType->range );

    // Erase us from the registry.
    {
        // Yea, I do use auto for this project.
        auto typeIter = std::find( this->types.begin(), this->types.end(), *streamType );

        assert( typeIter != this->types.end() );

        this->types.erase( typeIter );

        streamType = NULL;  // not valid anymore.
    }

    return true;
}

bool StreamMan::LinkResource( ident_t resID, std::string name, ResourceLocation *loc )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxLinkResource( this->lockResourceContest );

    // Is the slot already taken? Then fail.
    {
        resMap_t::const_iterator foundIter = this->resources.find( resID );

        if ( foundIter != this->resources.end() )
        {
            return false;
        }
    }

    // We want to occupy a Streaming Slot with actual resource data.
    Resource newLink;
    newLink.name = name;
    newLink.location = loc;
    newLink.status = eResourceStatus::UNLOADED;

    newLink.resourceSize = loc->getDataSize();

    this->resources[ resID ] = std::move( newLink );

    return true;
}
// evil meow.
bool StreamMan::UnlinkResourceNative( ident_t resID )
{
    bool hasUnlinked = false;

    // Find this resource.
    // If we found it, then reset this slot.
    {
        resMap_t::iterator iter = this->resources.find( resID );

        if ( iter != this->resources.end() )
        {
            // OK!
            this->resources.erase( iter );

            hasUnlinked = true;
        }
    }

    return hasUnlinked;
}

bool StreamMan::UnlinkResource( ident_t resID )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxUnlinkResource( this->lockResourceContest );

    return UnlinkResourceNative( resID );
}

};
}