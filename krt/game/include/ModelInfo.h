#pragma once

// Management for model resources in our engine!

#include <Streaming.common.h>

#define MODEL_ID_BASE   0
#define MAX_MODELS      20000

namespace krt
{

// General model information management container.
struct ModelManager : public streaming::StreamingTypeInterface
{
    ModelManager( streaming::StreamMan& streaming );
    ~ModelManager( void );

    void RegisterResource( std::string name, vfs::DevicePtr device, std::string pathToRes );

    void LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize ) override;
    void UnloadResource( streaming::ident_t localID ) override;

    size_t GetObjectMemorySize( streaming::ident_t localID ) const override;

private:
    streaming::StreamMan& streaming;

    struct ModelResource
    {
        inline ModelResource( vfs::DevicePtr device, std::string pathToRes ) : vfsResLoc( device, pathToRes )
        {
            return;
        }

        inline ~ModelResource( void )
        {
            return;
        }

        streaming::ident_t id;

        DeviceResourceLocation vfsResLoc;
    };

    std::vector <ModelResource*> models;

    std::map <std::string, ModelResource*> modelMap;
};

}