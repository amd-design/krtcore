#ifndef _STREAMING_RUNTIME_
#define _STREAMING_RUNTIME_

#include <Windows.h>

namespace Streaming
{

typedef unsigned int ident_t;

struct StreamingTypeInterface abstract
{
    virtual void LoadResource( ident_t localID, const void *data, size_t dataSize ) = 0;
    virtual void UnloadResource( ident_t localID ) = 0;

    virtual size_t GetObjectMemorySize( ident_t localID ) const = 0;
};

struct ResourceLocation
{
    std::string globalPath;
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
        UNLOADED,
        LOADED,
        LOADING,
        BUFFERING
    };

    struct Resource
    {
        std::string name;
        eResourceStatus status;
        ResourceLocation location;

        // Meta-data.
        size_t resourceSize;
    };

    StreamMan( void );
    ~StreamMan( void );

    bool Request( ident_t id );
    void CancelRequest( ident_t id );

    eResourceStatus GetResourceStatus( ident_t id ) const;

    void GetStatistics( StreamingStats& statsOut ) const;

    bool RegisterResourceType( ident_t base, ident_t range, StreamingTypeInterface *intf );
    bool UnregisterResourceType( ident_t base );

private:
    std::map <ident_t, Resource> resources;

    struct reg_streaming_type
    {
        StreamingTypeInterface *manager;
        ident_t base;
        ident_t range;
    };

    std::vector <reg_streaming_type> types;

    struct channel_t
    {
        struct request_t
        {
            ident_t resID;
        };

        std::list <request_t> requests;

        HANDLE thread;
        SRWLOCK channelLock;
    };
};

}

#endif //_STREAMING_RUNTIME_ by Martin Turski