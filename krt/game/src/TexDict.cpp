#include "StdInc.h"
#include "TexDict.h"
#include "Game.h"

namespace krt
{

TextureManager::TextureManager( streaming::StreamMan& streaming ) : streaming( streaming )
{
    bool success = streaming.RegisterResourceType( TXD_START_ID, MAX_TXD, this );

    assert( success == true );
}

TextureManager::~TextureManager( void )
{
    // We expect that things cannot stream anymore at this point.

    // Delete all resource things.
    for ( TexDictResource *texDict : this->texDictList )
    {
        streaming.UnlinkResource( texDict->id );

        delete texDict;
    }

    // Clear addressable lists.
    this->texDictMap.clear();
    this->texDictList.clear();

    // Unregister our manager from the streaming system.
    streaming.UnregisterResourceType( TXD_START_ID );
}

void TextureManager::RegisterResource( std::string name, vfs::DevicePtr device, std::string pathToRes )
{
    // Create some resource entry.
    TexDictResource *resEntry = new TexDictResource( device, std::move( pathToRes ) );

    streaming::ident_t curID = TXD_START_ID + ( this->texDictList.size() );

    resEntry->id = curID;

    streaming.LinkResource( curID, name, &resEntry->vfsResLoc );

    // Store our thing.
    this->texDictMap.insert( std::make_pair( name, resEntry ) );

    this->texDictList.push_back( resEntry );
}

void TextureManager::LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize )
{
    TexDictResource *texEntry = this->texDictList[ localID ];

    // TODO: hook up with aap's stuff.
}

void TextureManager::UnloadResource( streaming::ident_t localID )
{
    TexDictResource *texEntry = this->texDictList[ localID ];

    // TODO: hook up with aap's stuff.
}

size_t TextureManager::GetObjectMemorySize( streaming::ident_t localID ) const
{
    // TODO.
    return 0;
}

}