#include <StdInc.h>

#include <Game.h>
#include <GameUniverse.h>

#include <vfs/Manager.h>
#include <vfs/RelativeDevice.h>

#include <CdImageDevice.h>

#include <Console.CommandHelpers.h>
#include <Console.h>

namespace krt
{
GameUniverse::GameUniverse(const GameConfiguration& configuration)
    : m_configuration(configuration)
{
}

GameUniverse::~GameUniverse()
{
	for (streaming::ident_t index : m_streamingIndices)
	{
		theGame->GetStreaming().UnlinkResource(index);
	}
}

void GameUniverse::Load()
{
	// mount a relative device pointing at the root
	vfs::DevicePtr device = std::make_shared<vfs::RelativeDevice>(m_configuration.rootPath);
	vfs::Mount(device, GetMountPoint());

	// enqueue pre-cached IMG files
	for (const auto& imageFile : m_configuration.imageFiles)
	{
		AddImage(imageFile);
	}

	// load configuration files
	for (const auto& configurationFile : m_configuration.configurationFiles)
	{
		LoadConfiguration(configurationFile);
	}

	// load IMG files
	// TODO: figure out how to defer this?
	/*for (const auto& imageFile : m_imageFiles)
	{
		console::ExecuteSingleCommand(ProgramArguments{ "load_cdimage", m_configuration.gameName, imageFile.primaryMount });
	}*/
}

void GameUniverse::LoadConfiguration(const std::string& relativePath)
{
	vfs::StreamPtr stream       = vfs::OpenRead(GetMountPoint() + relativePath);
	std::vector<uint8_t> string = stream->ReadToEnd();

	console::Context localConsole;

	// add commands to the context
	ConsoleCommand ideLoadCmd(&localConsole, "IDE",
	                          [&](const std::string& fileName) {
		                          localConsole.ExecuteSingleCommand(ProgramArguments{"load_ide", m_configuration.gameName, GetMountPoint() + fileName});
		                      });

	ConsoleCommand iplLoadCmd(&localConsole, "IPL",
	                          [&](const std::string& fileName) {
		                          localConsole.ExecuteSingleCommand(ProgramArguments{"load_ipl", m_configuration.gameName, GetMountPoint() + fileName});
		                      });

	ConsoleCommand imgMountCmd(&localConsole, "IMG",
	                           [&](const std::string& path) {
		                           localConsole.ExecuteSingleCommand(ProgramArguments{"add_cdimage", m_configuration.gameName, path});
		                       });

	// run the configuration file
	localConsole.AddToBuffer(std::string(reinterpret_cast<char*>(string.data()), string.size()));
	localConsole.ExecuteBuffer();
}

void GameUniverse::AddImage(const std::string& relativePath)
{
	std::shared_ptr<streaming::CdImageDevice> cdImage = std::make_shared<streaming::CdImageDevice>();

	// if opening the image succeeded
	std::string imagePath = GetMountPoint() + relativePath;
	std::string mountPath = imagePath.substr(0, imagePath.find_last_of('.')) + "/";

	if (cdImage->OpenImage(imagePath))
	{
		// create a relative mount referencing the CD image and mount it
		vfs::DevicePtr relative = std::make_shared<vfs::RelativeDevice>(cdImage, mountPath);
		vfs::Mount(cdImage, mountPath);
		vfs::Mount(relative, GetImageMountPoint());

		// and add an entry to the list
		ImageFile entry;
		entry.cdimage       = cdImage;
		entry.relativeMount = relative;
		entry.primaryMount  = mountPath;

		m_imageFiles.push_back(entry);

		// pass the image file to be loaded immediately
		console::ExecuteSingleCommand(ProgramArguments{"load_cdimage", m_configuration.gameName, entry.primaryMount});
	}
}

std::string GameUniverse::GetMountPoint() const
{
	return m_configuration.gameName + ":/";
}

std::string GameUniverse::GetImageMountPoint() const
{
	return m_configuration.gameName + "img:/";
}
}