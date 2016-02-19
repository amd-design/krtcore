#pragma once

// Game things go here!

#define GAME_NUM_STREAMING_CHANNELS 4

#include "vfs\Device.h"

#include "ModelInfo.h"
#include "Streaming.h"
#include "TexDict.h"
#include "World.h"

#include "Console.Commands.h"
#include "GameUniverse.h"

namespace krt
{

class Game
{
	friend struct Entity;

  public:
	Game(void);
	~Game(void);

	void Run();

	std::string GetGamePath(std::string relPath);

	inline streaming::StreamMan& GetStreaming(void) { return this->streaming; }
	inline TextureManager& GetTextureManager(void) { return this->texManager; }
	inline ModelManager& GetModelManager(void) { return this->modelManager; }

	inline World* GetWorld(void) { return &theWorld; }

	inline float GetDelta() { return dT; }

	inline uint32_t GetLastFrameTime() { return lastFrameTime; }

	std::string GetDevicePathPrefix(void) { return "gta3:/"; }

	GameUniversePtr AddUniverse(const GameConfiguration& configuration);

	GameUniversePtr GetUniverse(const std::string& name);

  private:
	float dT;
	uint32_t lastFrameTime;

	std::string gameDir;

	streaming::StreamMan streaming;

	TextureManager texManager;
	ModelManager modelManager;

	World theWorld;

	NestedList<Entity> activeEntities;

	std::vector<GameUniversePtr> universes;
};

extern Game* theGame;

template <>
struct ConsoleArgumentType<GameUniversePtr>
{
	static bool Parse(const std::string& input, GameUniversePtr* out)
	{
		*out = theGame->GetUniverse(input);
		return true;
	}
};
};