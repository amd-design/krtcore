#include <StdInc.h>

#include <Game.h>
#include <GameUniverse.h>

#include <Console.CommandHelpers.h>

#include <FileLoader.h>

#include <vfs/Manager.h>

namespace krt
{
ConsoleCommand loadIdeCommand("load_ide", [](const GameUniversePtr& universe, const std::string& idePath) {
	FileLoader::LoadIDEFile(idePath, universe);
});

ConsoleCommand loadIplCommand("load_ipl", [](const GameUniversePtr& universe, const std::string& iplPath) {
	FileLoader::LoadIPLFile(iplPath, universe);
});

ConsoleCommand loadImgCommand("load_cdimage", [](const GameUniversePtr& universe, const std::string& mainMount) {
	FileLoader::ScanIMG(vfs::GetDevice(mainMount), mainMount, universe);
});
}