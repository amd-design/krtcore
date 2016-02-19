#pragma once

#include <GameUniverse.h>

namespace krt
{
class FileLoader
{
  public:
	// loads an IDE/IPL file without assigning to any universe
	inline static void LoadIDEFile(const std::string& absPath)
	{
		LoadIDEFile(absPath, GameUniversePtr());
	}

	inline static void LoadIPLFile(const std::string& absPath)
	{
		LoadIPLFile(absPath, GameUniversePtr());
	}

	// loads an IDE/IPL file, assigning it to an universe
	static void LoadIDEFile(const std::string& absPath, const GameUniversePtr& universe);
	static void LoadIPLFile(const std::string& absPath, const GameUniversePtr& universe);

	static void ScanIMG(const vfs::DevicePtr& device, const std::string& pathPrefix, const GameUniversePtr& universe);

  private:
	// static class only
	FileLoader();
};
}