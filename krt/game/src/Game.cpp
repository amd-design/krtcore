#include "StdInc.h"
#include "Game.h"

#include <cstdio>
#include <iostream>
#include <fstream>

#include "Console.h"
#include "Console.CommandHelpers.h"

#include "CdImageDevice.h"
#include "vfs/Manager.h"

#include <src/rwgta.h>

#pragma warning(disable:4996)

namespace krt
{

Game *theGame = NULL;

Game::Game( void ) : streaming( GAME_NUM_STREAMING_CHANNELS ), texManager( streaming ), modelManager( streaming, texManager )
{
    assert( theGame == NULL );

    // We can only have one game :)
    theGame = this;

    // Initialize RW.
    rw::platform = rw::PLATFORM_D3D9;
    rw::loadTextures = false;

    gta::attachPlugins();

    // Detect where the game is installed.
    // For now, I guess we do it manually.

    const char *computerName = getenv( "COMPUTERNAME" );

    if ( stricmp( computerName, "FALLARBOR" ) == 0 )
    {
        // Bas' thing.
        //this->gameDir = "S:\\Games\\CleanSA\\GTA San Andreas\\";
		this->gameDir = "S:\\Games\\Steam\\steamapps\\common\\Grand Theft Auto 3\\";
    }
    else if ( stricmp( computerName, "DESKTOP" ) == 0 )
    {
        // Martin's thing.
        this->gameDir = "D:\\gtaiso\\unpack\\gta3\\";
    }
    else
    {
        // Add your own, meow.
    }

    // Set up game related things.
    LIST_CLEAR( this->activeEntities.root );

	// Create the game universe.
	GameConfiguration configuration;
	configuration.gameName = "gta3";
	configuration.rootPath = gameDir;

	configuration.imageFiles.push_back("models/gta3.img");
	configuration.imageFiles.push_back("models/gta_int.img");

	configuration.configurationFiles.push_back("data/gta3.dat");

	GameUniversePtr universe = AddUniverse(configuration);
	universe->Load();

    // Do a test that loads all game models.
    modelManager.LoadAllModels();
}

Game::~Game( void )
{
    // Delete all our entities.
    {
        while ( !LIST_EMPTY( this->activeEntities.root ) )
        {
            Entity *entity = LIST_GETITEM( Entity, this->activeEntities.root.next, gameNode );

            // It will remove itself from the list.
            delete entity;
        }
    }

    // There is no more game.
    theGame = NULL;
}

GameUniversePtr Game::AddUniverse(const GameConfiguration& configuration)
{
	auto universe = std::make_shared<GameUniverse>(configuration);
	this->universes.push_back(universe);

	return universe;
}

GameUniversePtr Game::GetUniverse(const std::string& name)
{
	for (const GameUniversePtr& universe : this->universes)
	{
		if (universe->GetConfiguration().gameName == name)
		{
			return universe;
		}
	}

	return nullptr;
}

std::string Game::GetGamePath( std::string relPath )
{
    // Get some sort of relative directory from the game directory.
    // Note that we want the gameDir to have a slash at the end!
    return ( this->gameDir + relPath );
}

};