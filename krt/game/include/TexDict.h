#pragma once

// Texture Dictionary storage system.

#include "Streaming.common.h"

#define TXD_START_ID        20000
#define MAX_TXD             5000

namespace krt
{

struct TextureManager : public streaming::StreamingTypeInterface
{
    TextureManager( streaming::StreamMan& streaming );
    ~TextureManager( void );

    void RegisterResource( std::string name, vfs::DevicePtr device, std::string pathToRes );

    streaming::ident_t FindTexDict( const std::string& name ) const;
    void SetTexParent( const std::string& texName, const std::string& texParentName );

    void LoadResource( streaming::ident_t localID, const void *data, size_t dataSize ) override;
    void UnloadResource( streaming::ident_t localID ) override;

    size_t GetObjectMemorySize( streaming::ident_t localID ) const override;

private:
    streaming::StreamMan& streaming;

    struct TexDictResource
    {
        TexDictResource( vfs::DevicePtr device, std::string pathToRes ) : vfsResLoc( device, pathToRes )
        {
            return;
        }

        ~TexDictResource( void )
        {
            return;
        }

        streaming::ident_t id;
        streaming::ident_t parentID;

        DeviceResourceLocation vfsResLoc;

        // TODO: add pointer to aap's RW object.
    };

    TexDictResource* FindTexDictInternal( const std::string& name ) const;

    std::vector <TexDictResource*> texDictList;

    std::map <std::string, TexDictResource*> texDictMap;
};

}