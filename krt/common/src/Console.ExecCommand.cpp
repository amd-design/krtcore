#include <StdInc.h>

#include <vfs/Manager.h>
#include <Console.CommandHelpers.h>

namespace krt
{
static ConsoleCommand execCommand("exec", [] (const std::string& path)
{
	vfs::StreamPtr stream = vfs::OpenRead(path);

	if (!stream)
	{
		console::Printf("No such config file: %s\n", path.c_str());
		return;
	}

	std::vector<uint8_t> data = stream->ReadToEnd();
	data.push_back('\n'); // add a newline at the end

	console::AddToBuffer(std::string(reinterpret_cast<char*>(&data[0]), data.size()));
	console::ExecuteBuffer();
});
}