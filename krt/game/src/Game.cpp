#include "StdInc.h"
#include "Game.h"

#include <cstdio>
#include <fstream>
#include <iostream>

#include "Console.CommandHelpers.h"
#include "Console.h"

#include "CdImageDevice.h"
#include "vfs/Manager.h"

#include "EventSystem.h"

#include "sys/Timer.h"

#include <src/rwgta.h>

#pragma warning(disable : 4996)

namespace krt
{

Game* theGame = NULL;

Game::Game(void) : streaming(GAME_NUM_STREAMING_CHANNELS), texManager(streaming), modelManager(streaming, texManager)
{
	assert(theGame == NULL);

	// Initialize console variables.
	maxFPSVariable = std::make_unique<ConVar<int>>("maxFPS", ConVar_Archive, 60, &maxFPS);
	timescaleVariable = std::make_unique<ConVar<float>>("timescale", ConVar_None, 1.0f, &timescale);

	// We can only have one game :)
	theGame = this;

	// Initialize RW.
	rw::platform     = rw::PLATFORM_D3D9;
	rw::loadTextures = false;

	gta::attachPlugins();

	// Detect where the game is installed.
	// For now, I guess we do it manually.

	const char* computerName = getenv("COMPUTERNAME");

	if (stricmp(computerName, "FALLARBOR") == 0)
	{
		// Bas' thing.
		//this->gameDir = "S:\\Games\\CleanSA\\GTA San Andreas\\";
		this->gameDir = "S:\\Games\\Steam\\steamapps\\common\\Grand Theft Auto 3\\";
	}
	else if (stricmp(computerName, "DESKTOP") == 0)
	{
		// Martin's thing.
		this->gameDir = "D:\\gtaiso\\unpack\\gta3\\";
	}
	else
	{
		// Add your own, meow.
	}

	// Set up game related things.
	LIST_CLEAR(this->activeEntities.root);

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
	//modelManager.LoadAllModels();
}

Game::~Game(void)
{
	// Delete all our entities.
	{
		while (!LIST_EMPTY(this->activeEntities.root))
		{
			Entity* entity = LIST_GETITEM(Entity, this->activeEntities.root.next, gameNode);

			// It will remove itself from the list.
			delete entity;
		}
	}

	// There is no more game.
	theGame = NULL;
}

void Game::Run()
{
	sys::TimerContext timerContext;

	EventSystem eventSystem;

	// run the main game loop
	bool wantsToExit  = false;
	uint64_t lastTime = 0;

	while (!wantsToExit)
	{
		// limit frame rate and handle events
		// TODO: non-busy wait?
		int minMillis   = (1000 / this->maxFPS);
		uint32_t millis = 0;

		uint64_t thisTime = 0;

		do
		{
			thisTime = eventSystem.HandleEvents();

			millis = thisTime - lastTime;
		} while (millis < minMillis);

		// handle time scaling
		float scale = this->timescale; // to be replaced by a console value, again
		millis *= scale;

		if (millis < 1)
		{
			millis = 1;
		}
		// prevent too big jumps from being made
		else if (millis > 5000)
		{
			millis = 5000;
		}

		if (millis > 500)
		{
			printf("long frame: %d millis\n", millis);
		}

		// store timing values for this frame
		this->dT            = millis / 1000.0f;
		this->lastFrameTime = millis;

		lastTime = thisTime;

		// execute the command buffer for the global console
		console::ExecuteBuffer();

		// whatever else might come to mind
	}
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

std::string Game::GetGamePath(std::string relPath)
{
	// Get some sort of relative directory from the game directory.
	// Note that we want the gameDir to have a slash at the end!
	return (this->gameDir + relPath);
}
};