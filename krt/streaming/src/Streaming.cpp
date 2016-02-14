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
        // PLEASE NOTE THAT THIS IS A VERY COMPLICATED POINTER THAT COULD EASILY BREAK
        // IF YOU DO NOT KNOW WHAT YOU ARE DOING.
        Resource *resToLoad = NULL;

        if ( hasRequest )
        {
            exclusive_lock_acquire <std::shared_timed_mutex> ctxResLoadAcquire( manager->lockResourceContest );

            Resource *wantedResource = manager->GetResourceAtID( request.resID );

            if ( wantedResource )
            {
                if ( request.reqType == Channel::eRequestType::LOAD )
                {
                    // Who cares if we are terminating?
                    if ( manager->isTerminating == false && wantedResource->isAllowedToLoad == true )
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
                else if ( request.reqType == Channel::eRequestType::UNLOAD )
                {
                    if ( wantedResource->status == eResourceStatus::LOADED )
                    {
                        // Well, we want to do some stuff with this resource it seems...
                        resToLoad = wantedResource;
                    }
                }
                else
                {
                    assert( 0 );
                }
            }
            else
            {
                assert( 0 );
            }
        }

        if ( resToLoad )
        {
            manager->NativeProcessStreamingRequest( request, channel, resToLoad );
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

void StreamMan::NativeProcessStreamingRequest( const Channel::request_t& request, Channel *loadingChannel, Resource *resToLoad )
{
    shared_lock_acquire <std::shared_timed_mutex> ctxLoadResource( this->lockResourceContest );
            
    // Get the resource that we are meant to load.
    reg_streaming_type *typeInfo = this->GetStreamingTypeAtID( request.resID );

    if ( typeInfo )
    {
        StreamingTypeInterface *streamingType = typeInfo->manager;

        size_t resourceSize = resToLoad->resourceSize;

        ident_t localID = ( request.resID - typeInfo->base );

        if ( request.reqType == Channel::eRequestType::LOAD )
        {
            assert( loadingChannel != NULL );

            // Ensure that we have enough space in our loading buffer to store the resource.
            {
                if ( loadingChannel->dataBuffer.size() < resourceSize )
                {
                    loadingChannel->dataBuffer.resize( resourceSize );
                }
            }

            void *dataBuffer = loadingChannel->dataBuffer.data();

            // Load this resource.
            resToLoad->location->fetchData( dataBuffer );

            // Transition state from BUFFERING to LOADING.
            resToLoad->status = eResourceStatus::LOADING;

            // Give this data to the runtime.
            streamingType->LoadResource( localID, dataBuffer, resourceSize );

            // We are now loaded!
            resToLoad->status = eResourceStatus::LOADED;

            this->totalStreamingMemoryUsage += resourceSize;
        }
        else if ( request.reqType == Channel::eRequestType::UNLOAD )
        {
            // Just unload this crap.
            streamingType->UnloadResource( localID );

            // We are not loaded anymore, meow.
            this->totalStreamingMemoryUsage -= resourceSize;

            // Unloaded :)
            resToLoad->status = eResourceStatus::UNLOADED;
        }
        else
        {
            assert( 0 );
        }
    }
}

StreamMan::Channel::Channel( StreamMan *manager )
{
    this->manager = manager;

    channel_param *params = new channel_param;
    params->manager = manager;
    params->channel = this;

    // Semaphore for request availability.
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
    this->isTerminating = false;

    // Spawn channels for loading.
    for ( unsigned int n = 0; n < numChannels; n++ )
    {
        this->channels.push_back( new Channel( this ) );
    }
}

StreamMan::~StreamMan( void )
{
    this->isTerminating = true;

    // Clear all channels.
    // We just want to do things on the main thread.
    {
        for ( Channel *channel : this->channels )
        {
            delete channel;
        }

        this->channels.clear();
    }

    // Unload all resources.
    for ( std::pair <const ident_t, Resource>& loadedRes : this->resources )
    {
        Channel::request_t unloadReq;
        unloadReq.reqType = Channel::eRequestType::UNLOAD;
        unloadReq.resID = loadedRes.first;

        NativeProcessStreamingRequest( unloadReq, NULL, &loadedRes.second );
    }

    // Anything else?

    assert( this->totalStreamingMemoryUsage == 0 );
}

void StreamMan::NativePushChannelRequest( Channel *channel, Channel::request_t request )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxPushChannelRequest( channel->channelLock );

    assert( channel != NULL );

    channel->requests.push_back( std::move( request ) );

    // We have to lock the channel as active.
    // This is required to properly wait until it has finished loading.
    channel->isActive = true;

    // Notify the channel that a new request is available!
    ReleaseSemaphore( channel->semRequestCount, 1, NULL );
}

void StreamMan::NativePushStreamingRequest( Channel::request_t request )
{
    size_t channelCount = this->channels.size();

    unsigned int selChannel = ( this->currentChannelID++ % channelCount );

    // Give this channel the request.
    {
        Channel *channel = this->channels[ selChannel ];

        NativePushChannelRequest( channel, std::move( request ) );
    }
}

bool StreamMan::Request( ident_t id )
{
    // Don't allow requests if we are terminating.
    if ( isTerminating )
        return false;

    // Send this resource loading request to an available Channel.
    // Channels take loading requests on their own threads and provide data to the engine.

    shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( this->lockResourceContest );

    Channel::request_t newRequest;
    newRequest.reqType = Channel::eRequestType::LOAD;
    newRequest.resID = id;

    NativePushStreamingRequest( newRequest );

    return true;
}

bool StreamMan::Unload( ident_t id )
{
    // We do not want to handle unloading on the main thread, because it might be pretty heavy too!

    shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( this->lockResourceContest );
    
    Channel::request_t newRequest;
    newRequest.reqType = Channel::eRequestType::UNLOAD;
    newRequest.resID = id;

    NativePushStreamingRequest( newRequest );

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
    for ( Channel *curChannel : this->channels )
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
        this->UnlinkResourceNative( curID, false );
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
    newLink.isAllowedToLoad = true;

    newLink.resourceSize = loc->getDataSize();

    this->resources.insert( std::pair <ident_t, Resource> ( resID, std::move( newLink ) ) );

    return true;
}
// :)
bool StreamMan::UnlinkResourceNative( ident_t resID, bool doLock )
{
    bool hasUnlinked = false;

    // Find this resource.
    // If we found it, then reset this slot.
    {
        resMap_t::iterator iter = this->resources.find( resID );

        if ( iter != this->resources.end() )
        {
            // We have to uninstance this resource, if it is instanced.
            // Do that safely pls.
            {
                Resource *res = &iter->second;

                // Block this resource from transitioning into a loaded state.
                // This effectively prevents the loader from interefering with our unload-process.
                res->isAllowedToLoad = false;

                bool doesNeedUnload;
                {
                    if ( doLock )
                    {
                        this->lockResourceContest.lock_shared();
                    }

                    doesNeedUnload = ( res->status != eResourceStatus::UNLOADED );

                    if ( doLock )
                    {
                        this->lockResourceContest.unlock_shared();
                    }
                }

                if ( doesNeedUnload )
                {
                    Channel::request_t unloadRequest;
                    unloadRequest.reqType = Channel::eRequestType::UNLOAD;
                    unloadRequest.resID = resID;

                    NativePushStreamingRequest( std::move( unloadRequest ) );

                    // Wait for it to finish unloading.
                    while ( res->status != eResourceStatus::UNLOADED )
                    {
                        this->LoadingBarrier();
                    }
                }

                // Checking for unloaded status is very important.
                // If a resource is in the BUFFERING or LOADING state, it is being
                // managed by a streaming thread, which is very dangerous because
                // we cannot free the resource memory yet. The unload task above
                // takes care of such a hazard.
                assert( res->status == eResourceStatus::UNLOADED );
            }

            // We need to grab this lock because resources could be tried to be accessed
            // while we are erasing them. Erasing them under locks removes this risk.
            if ( doLock )
            {
                this->lockResourceContest.lock();
            }

            // OK!
            this->resources.erase( iter );

            if ( doLock )
            {
                this->lockResourceContest.unlock();
            }

            hasUnlinked = true;
        }
    }

    return hasUnlinked;
}

bool StreamMan::UnlinkResource( ident_t resID )
{
    return UnlinkResourceNative( resID, true );
}

};
}