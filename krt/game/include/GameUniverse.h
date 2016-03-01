#pragma once

#include <Streaming.h>

#include <vfs/Device.h>

#include <Console.CommandHelpers.h>

namespace krt
{
struct GameConfiguration
{
	// the internal name of the game ('gta3', for instance)
	std::string gameName;

	// the path (in the existing VFS tree) to the game root
	std::string rootPath;

	// a list of game configuration files (world DAT files) to load, relative to the game root
	std::vector<std::string> configurationFiles;

	// a list of CD images to preload (say, 'models/gta3.img')
	std::vector<std::string> imageFiles;
};

class Game;

// a game universe represents a single game's asset configuration
class GameUniverse
{
public:
	GameUniverse(const GameConfiguration& configuration);

	~GameUniverse();

	inline const GameConfiguration& GetConfiguration() const
	{
		return m_configuration;
	}

	void Load();

	std::string GetMountPoint() const;

	std::string GetImageMountPoint() const;

	inline void RegisterOwnedStreamingIndex(streaming::ident_t id)
	{
		m_streamingIndices.push_back(id);
	}

	inline void RegisterModelIndexMapping(streaming::ident_t from, streaming::ident_t to)
	{
		m_modelIndexMapping.insert({from, to});
	}

	inline streaming::ident_t GetModelIndexMapping(streaming::ident_t localId)
	{
		auto it = m_modelIndexMapping.find(localId);

#if _DEBUG
		// We kind of do not want this to happen in our testing.
		// But if it does, then handle it appropriately.
		assert(it != m_modelIndexMapping.end());
#endif

		if (it == m_modelIndexMapping.end())
			return -1;

		return it->second;
	}

private:
	void AddImage(const std::string& relativePath);

	ConsoleCommand cmdAddImage;

	void LoadConfiguration(const std::string& relativePath);

private:
	struct ImageFile
	{
		vfs::DevicePtr cdimage;
		vfs::DevicePtr relativeMount;

		// the path the CD image is primarily mounted at
		std::string primaryMount;
	};

private:
	GameConfiguration m_configuration;

	std::vector<ImageFile> m_imageFiles;

	std::vector<streaming::ident_t> m_streamingIndices;

	std::map<streaming::ident_t, streaming::ident_t> m_modelIndexMapping;

	Game* m_game;
};

using GameUniversePtr = std::shared_ptr<GameUniverse>;
}