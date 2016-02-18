#include "StdInc.h"
#include "TexDict.h"
#include "Game.h"

#include "NativePerfLocks.h"

namespace krt
{

TextureManager::TextureManager( streaming::StreamMan& streaming ) : streaming( streaming )
{
    // WARNING: there can only be ONE texture manager!
    // This is because of TLS things, and because we dont have a sophisticated enough treading library that can abstract problems away.

    bool success = streaming.RegisterResourceType( TXD_START_ID, MAX_TXD, this );

    assert( success == true );

    // We register our texture find callback and use it solely from here on.
    this->_tmpStoreOldCB = rw::Texture::findCB;

    rw::Texture::findCB = _rwFindTextureCallback;

    LIST_CLEAR( this->_tlCurrentTXDEnvList.root );
}

TextureManager::~TextureManager( void )
{
    // Unregister all current TXD environments.
    {
        exclusive_lock_acquire <std::shared_timed_mutex> ctxUnregisterTXDEnvs( this->lockTXDEnvList );

        while ( !LIST_EMPTY( this->_tlCurrentTXDEnvList.root ) )
        {
            ThreadLocal_CurrentTXDEnv *env = LIST_GETITEM(ThreadLocal_CurrentTXDEnv, this->_tlCurrentTXDEnvList.root.next, node);

            LIST_REMOVE( env->node );

            env->isRegistered = false;

            env->manager = NULL;
        }
    }

    // Unregister our find CB.
    rw::Texture::findCB = this->_tmpStoreOldCB;

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

    streaming::ident_t curID = TXD_START_ID + ( (streaming::ident_t)this->texDictList.size() );

    resEntry->id = curID;
    resEntry->parentID = -1;
    resEntry->txdPtr = NULL;

    resEntry->lockResourceLoad = SRWLOCK_INIT;

    bool couldLink = streaming.LinkResource( curID, name, &resEntry->vfsResLoc );

    // German radio is really really low quality with songs that are sung by wanna-be musicians.
    // I heavily dislike those new sarcastic songs. It is one reason why I dislike the German society aswell.
    // Not to say that the German society is honor based, which makes it good again.

    if ( !couldLink )
    {
        // Dang, something failed. Bail :/
        delete resEntry;

        return;
    }

    // Store our thing.
    this->texDictMap.insert( std::make_pair( name, resEntry ) );

    this->texDictList.push_back( resEntry );
}

TextureManager::ThreadLocal_CurrentTXDEnv* TextureManager::GetCurrentTXDEnv( void )
{
    static thread_local ThreadLocal_CurrentTXDEnv txdEnv( this );

    return &txdEnv;
}

rw::Texture* TextureManager::_rwFindTextureCallback( const char *name )
{
    Game *theGame = krt::theGame;

    if ( theGame )
    {
        TextureManager& texManager = theGame->GetTextureManager();

        ThreadLocal_CurrentTXDEnv *txdEnv = texManager.GetCurrentTXDEnv();

        if ( txdEnv )
        {
            // TODO: add parsing of parent TXD archives.

            rw::TexDictionary *currentTXD = txdEnv->currentTXD;

            if ( currentTXD )
            {
                return currentTXD->find( name );
            }
        }
    }

    return NULL;
}

TextureManager::TexDictResource* TextureManager::FindTexDictInternal( const std::string& name ) const
{
    auto findIter = this->texDictMap.find( name );

    if ( findIter == this->texDictMap.end() )
        return NULL;

    TexDictResource *texRes = findIter->second;

    return texRes;
}

streaming::ident_t TextureManager::FindTexDict( const std::string& name ) const
{
    TexDictResource *texRes = FindTexDictInternal( name );

    if ( !texRes )
    {
        return -1;
    }

    return texRes->id;
}

void TextureManager::SetTexParent( const std::string& texName, const std::string& texParentName )
{
    TexDictResource *texDict = this->FindTexDictInternal( texName );

    if ( !texDict )
        return;

    NativeSRW_Exclusive ctxSetParent( texDict->lockResourceLoad );

    TexDictResource *texParentDict = this->FindTexDictInternal( texParentName );

    if ( !texParentDict )
        return;

    NativeSRW_Shared ctxIsParent( texParentDict->lockResourceLoad );

    // Remove any previous link.
    streaming::ident_t mainID = texDict->id;
    {
        streaming::ident_t prevParent = texDict->parentID;

        if ( prevParent != -1 )
        {
            streaming.RemoveResourceDependency( mainID, prevParent );

            texDict->parentID = -1;
        }
    }

    // Establish the link, if possible.
    streaming::ident_t newParent = texParentDict->id;

    bool couldLink = streaming.AddResourceDependency( mainID, newParent );

    if ( !couldLink )
    {
        // meow.
        return;
    }

    // Store the relationship.
    texDict->parentID = newParent;

    // Success.
    return;
}

void TextureManager::SetCurrentTXD( streaming::ident_t id )
{
    if ( id < TXD_START_ID || id >= TXD_START_ID + this->texDictList.size() )
        return;

    id -= TXD_START_ID;

    TexDictResource *texDict = this->texDictList[ id ];

    if ( texDict == NULL )
        return;

    NativeSRW_Shared ctxSetCurrentTXD( texDict->lockResourceLoad );

    // Set it as current TXD.
    {
        rw::TexDictionary *txdPtr = texDict->txdPtr;

        if ( txdPtr )
        {
            ThreadLocal_CurrentTXDEnv *txdEnv = this->GetCurrentTXDEnv();

            txdEnv->currentTXD = txdPtr;
        }
    }
}

void TextureManager::UnsetCurrentTXD( void )
{
    ThreadLocal_CurrentTXDEnv *txdEnv = this->GetCurrentTXDEnv();

    if ( txdEnv )
    {
        txdEnv->currentTXD = NULL;
    }
}

void TextureManager::LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize )
{
    TexDictResource *texEntry = this->texDictList[ localID ];

    assert( texEntry != NULL );

    NativeSRW_Exclusive ctxLoadTXD( texEntry->lockResourceLoad );

    // Load the TXD resource.
    rw::TexDictionary *txdRes = NULL;
    {
        rw::StreamMemory memoryStream;
        memoryStream.open( (rw::uint8*)dataBuf, (rw::uint32)memSize );

        bool foundTexDict = rw::findChunk( &memoryStream, rw::ID_TEXDICTIONARY, NULL, NULL );

        if ( !foundTexDict )
        {
            throw std::exception( "not a TXD resource" );
        }

        txdRes = rw::TexDictionary::streamRead( &memoryStream );

        if ( !txdRes )
        {
            throw std::exception( "failed to parse texture dictionary file" );
        }
    }

    // Store it :)
    texEntry->txdPtr = txdRes;
}

void TextureManager::UnloadResource( streaming::ident_t localID )
{
    TexDictResource *texEntry = this->texDictList[ localID ];

    assert( texEntry != NULL );

    NativeSRW_Exclusive( texEntry->lockResourceLoad );

    // Unload the TXD again.
    {
        rw::TexDictionary *txdObj = texEntry->txdPtr;

        assert( txdObj != NULL );

        // Make sure we dont have this TXD object as active current TXD or something.
        {
            shared_lock_acquire <std::shared_timed_mutex> ctxCleanUpCurrentTXD( this->lockTXDEnvList );

            LIST_FOREACH_BEGIN(ThreadLocal_CurrentTXDEnv, this->_tlCurrentTXDEnvList.root, node)

                if ( item->currentTXD == txdObj )
                {
                    item->currentTXD = NULL;
                }

            LIST_FOREACH_END
        }

        txdObj->destroy();
    }

    texEntry->txdPtr = NULL;
}

size_t TextureManager::GetObjectMemorySize( streaming::ident_t localID ) const
{
    // TODO.
    return 0;
}

}