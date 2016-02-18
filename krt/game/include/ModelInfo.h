#pragma once

// Management for model resources in our engine!

#include <Streaming.common.h>

#include "TexDict.h"

#define MODEL_ID_BASE   0
#define MAX_MODELS      20000

namespace krt
{

// General model information management container.
struct ModelManager : public streaming::StreamingTypeInterface
{
    enum class eModelType
    {
        ATOMIC,
        VEHICLE,
        PED
    };
    
    struct ModelResource
    {
        inline streaming::ident_t GetID( void ) const           { return this->id; }
        inline eModelType GetType( void ) const                 { return this->modelType; }

        rw::Object* CloneModel( void );
        void ReleaseModel( rw::Object *rwobj );

    private:
        friend struct ModelManager;

        static void NativeReleaseModel( rw::Object *rwobj );

        inline ModelResource( vfs::DevicePtr device, std::string pathToRes ) : vfsResLoc( device, pathToRes )
        {
            return;
        }

        inline ~ModelResource( void )
        {
            return;
        }

        ModelManager *manager;

        streaming::ident_t id;
        streaming::ident_t texDictID;
        float lodDistance;
        int flags;

        eModelType modelType;

        DeviceResourceLocation vfsResLoc;

        rw::Object *modelPtr;

        SRWLOCK_VIRTUAL lockModelLoading;
    };

    ModelManager( streaming::StreamMan& streaming, TextureManager& texManager );
    ~ModelManager( void );

    void RegisterAtomicModel(
        streaming::ident_t id,
        std::string name, std::string texDictName, float lodDistance, int flags,
        std::string relFilePath
    );

    ModelResource* GetModelByID( streaming::ident_t id );

    void LoadAllModels( void );

    void LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize ) override;
    void UnloadResource( streaming::ident_t localID ) override;

    size_t GetObjectMemorySize( streaming::ident_t localID ) const override;

private:
    streaming::StreamMan& streaming;
    TextureManager& texManager;

    std::vector <ModelResource*> models;

    std::map <std::string, ModelResource*> modelByName;
};

}