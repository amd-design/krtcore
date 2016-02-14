#pragma once

#include <utils/NestedLList.h>

#include <atomic>

#include <shared_mutex>

#include <utils/DataSlice.h>

// Windows placeholder
typedef void* HANDLE;

namespace krt
{
namespace streaming
{

typedef int ident_t;

struct StreamingTypeInterface abstract
{
    virtual void LoadResource( ident_t localID, const void *data, size_t dataSize ) = 0;
    virtual void UnloadResource( ident_t localID ) = 0;

    virtual size_t GetObjectMemorySize( ident_t localID ) const = 0;
};

// Generic resource location provider!
struct ResourceLocation abstract
{
    // Returns the data size that will be written to the data buffer.
    // You cannot change this property during the lifetime of the resource.
    virtual size_t getDataSize( void ) const = 0;

    // Requests data from this resource.
    // This routine might be heavily threaded.
    virtual void fetchData( void *dataBuf ) = 0;
};

struct StreamingStats
{
    size_t memoryInUse;
    size_t maxMemory;
};

struct StreamMan
{
    enum class eResourceStatus
    {
        UNLOADED,   // not being managed by anything.
        LOADED,     // available and not being managed explicitly.
        LOADING,    // being managed
        BUFFERING,  // being managed
        UNLOADING   // being managed
    };

    StreamMan( unsigned int numChannels );
    ~StreamMan( void );

    bool Request( ident_t id );
    bool CancelRequest( ident_t id );
    bool Unload( ident_t id );

    void LoadingBarrier( void );
    bool WaitForResource( ident_t id ) const;

    eResourceStatus GetResourceStatus( ident_t id ) const;

    void GetStatistics( StreamingStats& statsOut ) const;

    bool RegisterResourceType( ident_t base, ident_t range, StreamingTypeInterface *intf );
    bool UnregisterResourceType( ident_t base );

    bool LinkResource( ident_t resID, std::string name, ResourceLocation *loc );
    bool UnlinkResource( ident_t resID );

private:
    // METHODS THAT ARE PRIVATE ARE MEANT TO BE PRIVATE.
    // Be careful exposing anything because this is a threaded structure!

    static void StreamingChannelRuntime( void *ud );

    bool CheckTypeRegionConflict( ident_t base, ident_t range ) const;

    struct Channel;

    struct Resource
    {
        inline Resource( void )
        {}

        inline Resource( Resource&& right ) : status()
        {
            this->name = std::move( right.name );
            this->status = right.status.load();
            this->location = right.location;
            this->isAllowedToLoad = right.isAllowedToLoad;
            this->resourceSize = right.resourceSize;
            this->syncOwner = right.syncOwner;
        }

        std::string name;
        std::atomic <eResourceStatus> status;
        ResourceLocation *location;
        bool isAllowedToLoad;

        Channel *syncOwner;

        // Meta-data.
        size_t resourceSize;
    };

    typedef std::map <ident_t, Resource> resMap_t;

    resMap_t resources;

    mutable std::shared_timed_mutex lockResourceContest;
    // this lock has to be taken when the resource system state is changing.

    inline Resource* GetResourceAtID( ident_t id )
    {
        resMap_t::iterator foundIter = resources.find( id );

        if ( foundIter == resources.end() )
            return NULL;

        return &(*foundIter).second;
    }

    inline const Resource* GetConstResourceAtID( ident_t id ) const
    {
        resMap_t::const_iterator foundIter = resources.find( id );

        if ( foundIter == resources.end() )
            return NULL;

        return &(*foundIter).second;
    }

    typedef sliceOfData <ident_t> identSlice_t;

    struct reg_streaming_type
    {
        StreamingTypeInterface *manager;
        ident_t base;
        ident_t range;

        inline bool operator ==( const reg_streaming_type& right ) const
        {
            return ( this->base == right.base && this->range == right.range );
        }
    };

    reg_streaming_type* GetStreamingTypeAtID( ident_t id );

    void ClearResourcesAtSlot( ident_t resID, ident_t range );

    bool UnlinkResourceNative( ident_t resID, bool doLock );

    std::vector <reg_streaming_type> types;

    struct Channel
    {
        Channel( StreamMan *manager );
        ~Channel( void );

        enum class eRequestType
        {
            LOAD,
            UNLOAD
        };

        struct request_t
        {
            request_t( void )
            {
                this->resID = -1;
                this->reqType = eRequestType::UNLOAD;
            }

            inline bool operator ==( const request_t& right ) const
            {
                return ( this->resID == right.resID && this->reqType == right.reqType );
            }

            ident_t resID;
            eRequestType reqType;
        };

        StreamMan *manager;

        std::list <request_t> requests;

        std::thread thread;
        mutable std::shared_timed_mutex channelLock;  // access lock to fields.

        mutable std::mutex lockIsActive;
        mutable std::condition_variable condIsActive;

        mutable std::mutex lockReqProcess;
        mutable std::condition_variable condReqProcess;

        HANDLE semRequestCount;
        HANDLE terminationEvent;

        bool isActive;

        // FIELDS STARTING FROM HERE ARE PRIVATE TO CHANNEL THREAD.
        std::vector <char> dataBuffer;

        // FIELDS STARTING FROM HERE ARE ONLY WRITE-ABLE BY CHANNEL THREAD.
        struct Activity
        {
            request_t task;
            NestedListEntry <Activity> node;
        };

        NestedList <Activity> activities;

        // only THREAD-SAFE if called from EXLUSIVE-LOCK at channelLock
        inline Activity* AllocateActivity( request_t req )
        {
            Activity *task = new Activity;

            task->task = std::move( req );
            LIST_INSERT( this->activities.root, task->node );

            return task;
        }

        // only THREAD-SAFE if called from EXCLUSIVE-LOCK at channelLock
        inline void DeallocateActivity( Activity *task )
        {
            LIST_REMOVE( task->node );

            delete task;
        }

        // METHODS STARTING FROM HERE ARE SAFE FOR CALLING FROM OTHER THREADS.
        inline bool IsChannelDoing( request_t req ) const
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxCheckActivity( this->channelLock );

            LIST_FOREACH_BEGIN( Activity, this->activities.root, node )

                if ( item->task == req )
                    return true;

            LIST_FOREACH_END

            return false;
        }

        inline bool IsChannelProcessing( ident_t id ) const
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxCheckActivity( this->channelLock );

            LIST_FOREACH_BEGIN( Activity, this->activities.root, node )

                if ( item->task.resID == id )
                    return true;

            LIST_FOREACH_END

            return false;
        }
    };

    std::vector <Channel*> channels;

    void NativeProcessStreamingRequest( const Channel::request_t& request, Channel *loadingChannel, Resource *resToLoad );

    void NativePushChannelRequest( Channel *channel, Channel::request_t request );
    Channel* NativePushStreamingRequest( Channel::request_t request );

    void NativeChannelWaitForCompletion( Channel *channel ) const;
    bool NativeWaitForResourceActivity( ident_t id ) const;

    struct channel_param
    {
        StreamMan *manager;
        Channel *channel;
    };

    // Some management variables.
    std::atomic <unsigned int> currentChannelID;    // used to balance the load between channels.

    std::atomic <size_t> maxMemory;
    std::atomic <size_t> totalStreamingMemoryUsage;

    bool isTerminating;
};

}
}