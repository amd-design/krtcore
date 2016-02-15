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

void StreamMan::Channel::StreamingChannelRuntime( void *ud )
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

    // TODO: add ERROR HANDLING to the streaming runtimes so that we can gracefully
    // stop loading things if they just cannot be.

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
        Channel::Activity *mainActivity = NULL;
        bool hasRequest = false;
        Channel::request_t request;
        {
            std::unique_lock <std::mutex> ctxIsActiveUpdate( channel->lockIsActive );
            std::unique_lock <std::mutex> ctxReqProcUpdate( channel->lockReqProcess );

            exclusive_lock_acquire <std::shared_timed_mutex> ctxFetchRequest( channel->channelLock );

            if ( channel->requests.empty() == false )
            {
                request = channel->requests.front();

                channel->requests.pop_front();

                // We want to let the runtime know that we are doing something.
                mainActivity = channel->AllocateActivity( request );

                hasRequest = true;
            }
        }

        // We have to check whether this request makes any sense.
        // Do that in a minimal verification phase.
        // PLEASE NOTE THAT THIS IS A VERY COMPLICATED POINTER THAT COULD EASILY BREAK
        // IF YOU DO NOT KNOW WHAT YOU ARE DOING.
        Resource *resToLoad = NULL;
        eRequestType reqType = request.reqType;

        if ( hasRequest )
        {
            exclusive_lock_acquire <std::shared_timed_mutex> ctxResLoadAcquire( manager->lockResourceContest );

            Resource *wantedResource = manager->GetResourceAtID( request.resID );

            if ( wantedResource )
            {
                resToLoad = channel->AcquireResourceContext( wantedResource, request.reqType );
            }
        }

        if ( resToLoad )
        {
            channel->ProcessResourceRequest( manager, resToLoad, reqType );
        }

        // Need to clear activity flag.
        if ( hasRequest )
        {
            // This is actually the counter part to acquiring a resource context.

            std::unique_lock <std::mutex> ctxIsActiveUpdate( channel->lockIsActive );
            std::unique_lock <std::mutex> ctxReqProcUpdate( channel->lockReqProcess );

            exclusive_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( channel->channelLock );

            // The resource is not being maintained anymore.
            if ( resToLoad )
            {
                resToLoad->syncOwner = NULL;
            }

            // We are not persuing our main goal anymore.
            channel->DeallocateActivity( mainActivity );

            mainActivity = NULL;

            // We are not active anymore if all requests are fulfilled.
            if ( channel->requests.empty() == true )
            {
                channel->isActive = false;

                // We can unlock any waiting people.
                channel->condIsActive.notify_all();
            }

            // Notify some other people that a resource finished processing.
            channel->condReqProcess.notify_all();
        }
    }

    return;
}

// only THREAD-SAFE if called from EXCLUSIVE-ACCESS at lockResourceContest !
StreamMan::Resource* StreamMan::Channel::AcquireResourceContext( Resource *wantedResource, eRequestType reqType )
{
    Resource *resToLoad = NULL;

    if ( reqType == eRequestType::LOAD )
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
    else if ( reqType == eRequestType::UNLOAD )
    {
        // Cannot unload if another task holds a reference to this.
        if ( wantedResource->refCount == 0 )
        {
            if ( wantedResource->status == eResourceStatus::LOADED )
            {
                // New state: unloading!
                wantedResource->status = eResourceStatus::UNLOADING;

                // Well, we want to do some stuff with this resource it seems...
                resToLoad = wantedResource;
            }
        }
    }
    else
    {
        assert( 0 );
    }

    // Each resource can be owned by one sync channel.
    // The channel is basically a worker thread maintaining resources.
    if ( resToLoad )
    {
        assert( resToLoad->syncOwner == NULL );

        resToLoad->syncOwner = this;
    }

    return resToLoad;
}

void StreamMan::Channel::ProcessResourceRequest( StreamMan *manager, Resource *resToLoad, eRequestType reqType )
{
    // Make sure that our dependencies cannot unload.
    // This makes sense because dependencies are there to stay for as long as the resource lives.
    if ( reqType == eRequestType::LOAD )
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxDependsAvailability( manager->lockResourceAvail );

        shared_lock_acquire <std::shared_timed_mutex> ctxTraverseDependencies( manager->lockDependsMutate );

        resToLoad->CastDependencyLoadRequirement();
    }

    try
    {
        // Load all dependencies before attempting to load the main resource.
        // Note that exceptions during the loading process may kill the loading of the main resource.
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxDependsAvailability( manager->lockResourceAvail );

            shared_lock_acquire <std::shared_timed_mutex> ctxTraverseDependencies( manager->lockDependsMutate );

            manager->lockResourceContest.lock();

            try
            {
                for ( Resource *dependency : resToLoad->depends )
                {
                    eResourceStatus status = dependency->status;

                    if ( status != eResourceStatus::LOADED )
                    {
                        // Take care of the problem that another channel could be taking the work from us.
                        // Resources cannot be maintained infinitely by different channels, can they?
                        // If they are, then the problem is in another component of the engine being faulty.
                        while ( Channel *conflictingWorker = dependency->syncOwner )
                        {
                            // If this resource is being maintained by another channel, we wait till it has completed work.

                            // We can release this lock BECAUSE the resource dependencies are immutable
                            // AND the resToLoad object cannot be destroyed while we manage it.
                            manager->lockResourceContest.unlock();

                            manager->NativeChannelWaitForResourceCompletion( conflictingWorker, dependency->id );
                                
                            manager->lockResourceContest.lock();
                        }

                        // Status of the resource could have changed!
                        status = dependency->status;

                        if ( status == eResourceStatus::UNLOADED )
                        {
                            // We just want to load this resource.
                            Resource *dependToBeLoaded = this->AcquireResourceContext( dependency, eRequestType::LOAD );

                            // TODO: add error handling logic.
                            assert( dependToBeLoaded != NULL );

                            // Register an activity about what we are doing.
                            Activity *subActivity = NULL;
                            {
                                std::unique_lock <std::mutex> ctxIsActiveUpdate( this->lockIsActive );

                                exclusive_lock_acquire <std::shared_timed_mutex> ctxActivityUpdate( this->channelLock );

                                request_t subRequest;
                                subRequest.reqType = eRequestType::LOAD;
                                subRequest.resID = dependToBeLoaded->id;

                                subActivity = this->AllocateActivity( std::move( subRequest ) );
                            }

                            manager->lockResourceContest.unlock();

                            try
                            {
                                // Do the loading!
                                ProcessResourceRequest( manager, dependToBeLoaded, eRequestType::LOAD );
                            }
                            catch( ... )
                            {
                                manager->lockResourceContest.lock();

                                throw;
                            }

                            manager->lockResourceContest.lock();

                            // Not a sync owner anymore.
                            {
                                assert( dependToBeLoaded->syncOwner == this );

                                dependToBeLoaded->syncOwner = NULL;
                            }

                            // Unregister the sub activity again.
                            {
                                std::unique_lock <std::mutex> ctxIsActiveUpdate( this->lockIsActive );

                                exclusive_lock_acquire <std::shared_timed_mutex> ctxActivityUpdate( this->channelLock );

                                this->DeallocateActivity( subActivity );

                                // We finished some activity, so notify people.
                                this->condIsActive.notify_all();
                            }
                        }
                        else if ( status == eResourceStatus::LOADED )
                        {
                            // We are loaded already :)
                        }
                        else
                        {
                            // No idea what kind of state this is...
                            assert( 0 );
                        }
                    }
                }
            }
            catch( ... )
            {
                manager->lockResourceContest.unlock();

                throw;
            }

            manager->lockResourceContest.unlock();
        }

        // Load the main resource.
        manager->NativeProcessStreamingRequest( reqType, this, resToLoad );
    }
    catch( ... )
    {
        if ( reqType == eRequestType::LOAD )
        {
            // Have to clear dependency lock because we failed loading the resource.
            shared_lock_acquire <std::shared_timed_mutex> ctxDependsAvailability( manager->lockResourceAvail );

            shared_lock_acquire <std::shared_timed_mutex> ctxTraverseDependencies( manager->lockDependsMutate );

            resToLoad->UncastDependencyLoadRequirement();
        }

        // Pass on the exception :3
        throw;
    }

    // If we are unloading, we can get rid of the dependencies now.
    if ( reqType == eRequestType::UNLOAD )
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxDependsAvailability( manager->lockResourceAvail );

        shared_lock_acquire <std::shared_timed_mutex> ctxTraverseDependencies( manager->lockDependsMutate );

        resToLoad->UncastDependencyLoadRequirement();
    }
    // Else KEEP the dependency refCount to force the dependencies to stay loaded.
}

void StreamMan::NativeProcessStreamingRequest( Channel::eRequestType reqType, Channel *loadingChannel, Resource *resToLoad )
{
    ident_t resID = resToLoad->id;

    shared_lock_acquire <std::shared_timed_mutex> ctxLoadResource( this->lockResourceContest );
            
    // Get the resource that we are meant to load.
    reg_streaming_type *typeInfo = this->GetStreamingTypeAtID( resID );

    if ( typeInfo )
    {
        StreamingTypeInterface *streamingType = typeInfo->manager;

        size_t resourceSize = resToLoad->resourceSize;

        ident_t localID = ( resID - typeInfo->base );

        if ( reqType == Channel::eRequestType::LOAD )
        {
            assert( loadingChannel != NULL );

            // Request a private buffer from the channel.
            void *dataBuffer = loadingChannel->GetStreamingBuffer( resourceSize );

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
        else if ( reqType == Channel::eRequestType::UNLOAD )
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

    LIST_CLEAR( this->activities.root );

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

    assert( LIST_EMPTY( this->activities.root ) == true );

    assert( this->isActive == false );

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
    // Prevent anything from loading anymore.
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
        Resource *resToUnload = &loadedRes.second;

        if ( resToUnload->status == eResourceStatus::LOADED )
        {
            NativeProcessStreamingRequest( Channel::eRequestType::UNLOAD, NULL, &loadedRes.second );
        }
        else
        {
            assert( resToUnload->status == eResourceStatus::UNLOADED );
        }

        // Terminate some things about this resource.
        resToUnload->refCount = 0;
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

StreamMan::Channel* StreamMan::NativePushStreamingRequest( Channel::request_t request )
{
    size_t channelCount = this->channels.size();

    unsigned int selChannel = ( this->currentChannelID++ % channelCount );

    // Give this channel the request.
    Channel *channel = this->channels[ selChannel ];

    NativePushChannelRequest( channel, std::move( request ) );

    return channel;
}

void StreamMan::NativeChannelWaitForCompletion( Channel *channel ) const
{
    std::unique_lock <std::mutex> lockWaitActive( channel->lockIsActive );

    channel->condIsActive.wait( lockWaitActive,
        [&]
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( channel->channelLock );

        return ( ! ( channel->requests.empty() == false || channel->isActive ) );
    });
}

void StreamMan::NativeChannelWaitForResourceCompletion( Channel *channel, ident_t id ) const
{
    std::unique_lock <std::mutex> lockWaitActive( channel->lockReqProcess );

    channel->condReqProcess.wait( lockWaitActive,
        [&]
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxChannelConsistency( channel->channelLock );

        // Check if a certain resource is being in-the-queue or being worked on.
        for ( const Channel::request_t& req : channel->requests )
        {
            if ( req.resID == id )
            {
                // Wait.
                return false;
            }
        }

        // Being worked on?
        if ( channel->IsChannelProcessingNoLock( id ) )
        {
            return false;
        }

        // Nothing to worry about anymore!
        return true;
    });
}

bool StreamMan::NativeWaitForResourceActivity( ident_t id ) const
{
    // Determine which channel is currently processing this resource (if it even is being processed)
    // and wait for that channel to complete work.

    Channel *waitForChannel = NULL;

    // Try to get the channel from the queue of any worker thread.
    {
        for ( Channel *channel : this->channels )
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxCheckChannelQueue( channel->channelLock );

            for ( const Channel::request_t req : channel->requests )
            {
                if ( req.resID == id )
                {
                    // That channel has some business to do with this resource, so lets wait till it has finished said business.
                    waitForChannel = channel;
                }
            }
        }
    }

    if ( waitForChannel == NULL )
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxCheckResourceWorker( this->lockResourceContest );

        // Attempt to get the worker channel from the resource itself.
        const Resource *resToCheck = this->GetConstResourceAtID( id );

        eResourceStatus resStatus = resToCheck->status;

        if ( resStatus != eResourceStatus::UNLOADED && resStatus != eResourceStatus::LOADED )
        {
            // We want to wait for the channel that works on this to complete things.
            waitForChannel = resToCheck->syncOwner;
        }
    }

    bool didWait = false;

    if ( waitForChannel )
    {
        didWait = true;

        NativeChannelWaitForResourceCompletion( waitForChannel, id );
    }

    return didWait;
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
        NativeChannelWaitForCompletion( curChannel );
    }
}

bool StreamMan::WaitForResource( ident_t id ) const
{
    return NativeWaitForResourceActivity( id );
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
    exclusive_lock_acquire <std::shared_timed_mutex> ctxNativeResLink( this->lockResourceAvail );

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
    {
        Resource newLink( resID, std::move( name ), loc );

        this->resources.insert( std::pair <ident_t, Resource> ( resID, std::move( newLink ) ) );
    }

    return true;
}

bool StreamMan::UnlinkResourceNative( ident_t resID, bool doLock )
{
    bool hasUnlinked = false;

    // Find this resource.
    // If we found it, then reset this slot.
    {
        shared_lock_acquire <std::shared_timed_mutex> ctxResourceDeinitialize( this->lockResourceAvail );

        resMap_t::iterator iter = this->resources.find( resID );

        if ( iter != this->resources.end() )
        {
            // We have to uninstance this resource, if it is instanced.
            // Do that safely pls.
            Resource *res = &iter->second;
            {
                // Block this resource from transitioning into a loaded state.
                // This effectively prevents the loader from interfering with our unload-process.
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

                    Channel *reqChannel = NativePushStreamingRequest( std::move( unloadRequest ) );

                    // Wait for it to finish unloading.
                    while ( res->status != eResourceStatus::UNLOADED )
                    {
                        NativeChannelWaitForResourceCompletion( reqChannel, resID );
                    }
                }

                // Checking for unloaded status is very important.
                // If a resource is in the BUFFERING or LOADING state, it is being
                // managed by a streaming thread, which is very dangerous because
                // we cannot free the resource memory yet. The unload task above
                // takes care of such a hazard.
                assert( res->status == eResourceStatus::UNLOADED );
            }

            // Remove any dependencies from and to this resource.
            {
                exclusive_lock_acquire <std::shared_timed_mutex> ctxDependsMutate( this->lockDependsMutate );

                // Remove any back-links from dependencies.
                for ( Resource *dependency : res->depends )
                {
                    // We are very certain there must be a back-link!
                    dependency->RemoveDependingOnBackLink( res );
                }

                // Now we are safe to clear our own dependencies. :)
                res->depends.clear();
            }

            // From now on, this resource is secure to be "deinitialized".
            // Thus we can try to delete it in the next step.
        }
    }

    // Delete the resource.
    // Note that another thread could have solved this problem before us.
    {
        exclusive_lock_acquire <std::shared_timed_mutex> ctxResourceUnlink( this->lockResourceAvail );

        resMap_t::iterator iter = this->resources.find( resID );

        if ( iter != this->resources.end() )
        {
            Resource *resToDelete = &iter->second;

            assert( resToDelete->status == eResourceStatus::UNLOADED );

            assert( resToDelete->refCount == 0 );

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

bool StreamMan::AddResourceDependency( ident_t resID, ident_t dependsOn )
{
    // I assert that it is safe to mutate dependencies even if the loader is processing said resource.
    shared_lock_acquire <std::shared_timed_mutex> ctxResourceConsistency( this->lockResourceAvail );

    exclusive_lock_acquire <std::shared_timed_mutex> ctxMutateDependencies( this->lockDependsMutate );

    // TODO: actually handle edge cases...
    // what if resource is loaded already?

    // Add a nice dependency to nice resources.
    // Dependencies are used to chain-load resources so they can be sure that
    // certain engine components are there before they load.
    bool success = false;

    Resource *srcResource = GetResourceAtID( resID );

    if ( srcResource )
    {
        Resource *dependency = GetResourceAtID( dependsOn );
        
        if ( dependency )
        {
            // Verify that this is a valid dependency link.
            // Dependencies must not create loops/uproots.
            bool validLink = true;

            // * dependency must not depend on srcResource.
            {
                if ( dependency->DoesDependOn( srcResource ) )
                {
                    // This would create a circular dependency.
                    // We cannot allow that.
                    validLink = false;
                }
            }

            // * maybe we depend on this thing already?
            //   do note that sub resources are still allowed to have duplicates.
            {
                for ( Resource *alreadyDepends : srcResource->depends )
                {
                    if ( alreadyDepends == dependency )
                    {
                        // We dont want redundancies, so abort.
                        validLink = false;
                        break;
                    }
                }
            }

            if ( validLink )
            {
                // Keep track that dependency is being depended on by srcResource.
                dependency->dependingOn.push_back( srcResource );

                // And of course register the dependency for loading.
                srcResource->depends.push_back( dependency );

                success = true;
            }
        }
    }

    return success;
}

bool StreamMan::RemoveResourceDependency( ident_t resID, ident_t dependsOn )
{
    shared_lock_acquire <std::shared_timed_mutex> ctxResourceConsistency( this->lockResourceAvail );

    exclusive_lock_acquire <std::shared_timed_mutex> ctxMutateDependencies( this->lockDependsMutate );

    // TODO: think about obscure edge cases that need special handling.

    // We want to remove a dependency I guess.
    bool success = false;

    // Good news is that this is a lot less complicated than adding a dependency!
    Resource *srcResource = this->GetResourceAtID( resID );

    if ( srcResource )
    {
        Resource *dependency = this->GetResourceAtID( dependsOn );

        if ( dependency )
        {
            // Remove us, if we really depend on things.
            {
                auto findIter = std::find( srcResource->depends.begin(), srcResource->depends.end(), dependency );

                if ( findIter != srcResource->depends.end() )
                {
                    srcResource->depends.erase( findIter );

                    success = true;
                }
            }

            // Also unlink the depending-on back-link.
            if ( success )
            {
                dependency->RemoveDependingOnBackLink( srcResource );
            }
        }
    }

    return success;
}

};
}