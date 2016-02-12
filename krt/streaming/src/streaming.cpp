#include "StdInc.h"
#include "streaming.h"

using namespace NativeExecutive;

// NOTE that this is a very new system that needs ironing out bugs.
// So report any issue that you can find!

namespace Streaming
{

void StreamMan::StreamingChannelRuntime( CExecThread *theThread, void *ud )
{
    struct GuardedResourceWait : public NativeExecutive::hazardPreventionInterface
    {
        inline GuardedResourceWait( StreamMan *streamMan, CExecutiveManager *execMan, HANDLE semWait )
        {
            this->streamMan = streamMan;
            this->execMan = execMan;
            this->semWait = semWait;

            NativeExecutive::PushHazard( execMan, this );
        }

        inline ~GuardedResourceWait( void )
        {
            NativeExecutive::PopHazard( this->execMan );
        }

        void TerminateHazard( void ) override
        {
            // We want to give the thread a chance to terminate.
            ReleaseSemaphore( this->semWait, 1, NULL );
        }

        StreamMan *streamMan;
        CExecutiveManager *execMan;
        HANDLE semWait;
    };

    StreamMan *manager = NULL;
    Channel *channel = NULL;
    {
        const channel_param *params = (const channel_param*)ud;

        manager = params->manager;
        channel = params->channel;

        // We must clean up after ourselves.
        delete params;
    }
    
    CExecutiveManager *execMan = manager->execMan;

    while ( true )
    {
        // We must always check for termination, so we can cleanly exit.
        execMan->CheckHazardCondition();

        // Wait for requests to become available.
        {
            GuardedResourceWait waitGuard( manager, execMan, channel->semRequestCount );

            // Need to check again.
            execMan->CheckHazardCondition();

            // Friggin thread hazards.
            WaitForSingleObject( channel->semRequestCount, INFINITE );
        }

        // Take a request and fulfill it!
        bool hasRequest = false;
        Channel::request_t request;
        {
            CReadWriteWriteContext <CReadWriteLock> ctxFetchRequest( channel->channelLock );

            if ( channel->requests.empty() == false )
            {
                request = channel->requests.front();

                channel->requests.pop_front();

                hasRequest = true;

                // Need to set us as active.
                channel->isActive = true;
            }
        }

        // We have to check whether this request makes any sense.
        // Do that in a minimal verification phase.
        Resource *resToLoad = NULL;

        if ( hasRequest )
        {
            CReadWriteWriteContext <CReadWriteLock> ctxResLoadAcquire( manager->lockResourceContest );

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
            CReadWriteReadContext <CReadWriteLock> ctxLoadResource( manager->lockResourceContest );
            
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
            }
        }

        // Need to clear activity flag.
        if ( hasRequest )
        {
            channel->isActive = false;
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

    // Lock used to access the resource list and stuff.
    this->channelLock = manager->execMan->CreateReadWriteLock();

    // Semaphore request availability semaphore.
    this->semRequestCount = CreateSemaphoreW( NULL, 0, 9000, NULL );
    this->isActive = false;

    this->thread =
        manager->execMan->CreateThread( StreamingChannelRuntime, params );

    // Start the thread right away.
    this->thread->Resume();
}

StreamMan::Channel::~Channel( void )
{
    // Wait for thread termination.
    manager->execMan->TerminateThread( this->thread, true );

    manager->execMan->CloseThread( this->thread );

    this->thread = NULL;

    // Clean up management stuff.
    CloseHandle( this->semRequestCount );

    manager->execMan->CloseReadWriteLock( this->channelLock );
}

StreamMan::StreamMan( unsigned int numChannels )
{
    this->execMan = CExecutiveManager::Create();

    assert( this->execMan != NULL );

    // Initialize management variables.
    this->currentChannelID = 0;

    this->lockResourceContest = this->execMan->CreateReadWriteLock();

    // Spawn channels for loading.
    for ( unsigned int n = 0; n < numChannels; n++ )
    {
        Channel *newChannel = new Channel( this );

        this->channels.push_back( newChannel );
    }
}

StreamMan::~StreamMan( void )
{
    // Clear all channels.
    {
        for ( Channel *runningChannel : this->channels )
        {
            delete runningChannel;
        }

        this->channels.clear();
    }

    // Clear management things.
    this->execMan->CloseReadWriteLock( this->lockResourceContest );

    // Kill threading stuff.
    CExecutiveManager::Delete( this->execMan );
}

bool StreamMan::Request( ident_t id )
{
    // Send this resource loading request to an available Channel.
    // Channels take loading requests on their own threads and provide data to the engine.

    CReadWriteReadContext <CReadWriteLock> ctxChannelConsistency( this->lockResourceContest );

    size_t channelCount = this->channels.size();

    unsigned int selChannel = ( this->currentChannelID++ % channelCount );

    // Give this channel the request.
    {
        Channel *channel = this->channels[ selChannel ];

        CReadWriteWriteContext <CReadWriteLock> ctxPushChannelRequest( channel->channelLock );

        assert( channel != NULL );

        Channel::request_t newRequest;
        newRequest.resID = id;

        channel->requests.push_back( std::move( newRequest ) );

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
    bool areResourcesLoaded = false;

    while ( !areResourcesLoaded )
    {
        bool isAnyActivity = false;

        // Check all channels.
        for ( Channel *curChannel : this->channels )
        {
            CReadWriteReadContext <CReadWriteLock> ctxCheckChannelActivity( curChannel->channelLock );

            if ( curChannel->requests.empty() == false || curChannel->isActive )
            {
                isAnyActivity = true;
            }
        }

        if ( !isAnyActivity )
        {
            areResourcesLoaded = true;
        }
        else
        {
            // Let us relax for a second.
            Sleep( 1 );
        }
    }
}

StreamMan::eResourceStatus StreamMan::GetResourceStatus( ident_t id ) const
{
    return eResourceStatus::UNLOADED;
}

void StreamMan::GetStatistics( StreamingStats& statsOut ) const
{
    // TODO.
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
    CReadWriteWriteContext <CReadWriteLock> ctxRegisterType( this->lockResourceContest );

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
    CReadWriteWriteContext <CReadWriteLock> ctxUnregisterType( this->lockResourceContest );

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
    CReadWriteWriteContext <CReadWriteLock> ctxLinkResource( this->lockResourceContest );

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
    CReadWriteWriteContext <CReadWriteLock> ctxUnlinkResource( this->lockResourceContest );

    return UnlinkResourceNative( resID );
}

};