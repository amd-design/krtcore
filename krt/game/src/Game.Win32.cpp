#include "StdInc.h"
#include "Game.h"

#include <vfs/Manager.h>
#include <vfs/RelativeDevice.h>

#include <windows.h>

#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace krt
{
static void AppendPathComponent(std::wstring& value, const wchar_t* component)
{
	value += component;

	if (GetFileAttributesW(value.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		CreateDirectoryW(value.c_str(), nullptr);
	}
}

void Game::MountUserDirectory()
{
	PWSTR saveBasePath;

	// get the 'Saved Games' shell directory
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, nullptr, &saveBasePath)))
	{
		// create a STL string and free the used memory
		std::wstring savePath(saveBasePath);

		CoTaskMemFree(saveBasePath);

		// append our path components
		AppendPathComponent(savePath, L"\\AMDDesign");
		AppendPathComponent(savePath, L"\\ATG");

		// append a final separator
		savePath += L"\\";

		// and turn the path into a mount
		vfs::DevicePtr device = std::make_shared<vfs::RelativeDevice>(ToNarrow(savePath));
		vfs::Mount(device, "user:/");
	}
}

void Game::YieldThreadForShortTime()
{
	// I do not want this thing to max out my CPU :/
	// And we are not doing realistic simulations yet.
	Sleep(1);
}
}