#include "StdInc.h"
#include "RTMain.h"
#include "ProgramArguments.h"

#include <stdio.h>

#include <vfs/Manager.h>
#include <Streaming.h>

namespace krt
{
int unknown()
{
	vfs::StreamPtr stream = vfs::OpenRead("C:\\Windows\\system32\\License.rtf");
	auto data = stream->ReadToEnd();

	assert(false && "hey!");

	return 0;
}

class TestInterface : public streaming::StreamingTypeInterface
{
public:
	virtual void LoadResource(streaming::ident_t localID, const void *data, size_t dataSize) override
	{
		printf("%d: %s\n", localID, std::string(reinterpret_cast<const char*>(data), dataSize).c_str());
	}
	virtual void UnloadResource(streaming::ident_t localID) override
	{
		printf("%d-\n", localID);
	}

	virtual size_t GetObjectMemorySize(streaming::ident_t localID) const override
	{
		return 5;
	}
};

class VfsResourceProvider : public streaming::ResourceLocation
{
private:
	std::string m_path;
	vfs::DevicePtr m_device;

public:
	VfsResourceProvider(const std::string& path)
	{
		m_path = path;
		m_device = vfs::GetDevice(path);
	}

	virtual size_t getDataSize(void) const override
	{
		return m_device->GetLength(m_path);
	}

	virtual void fetchData(void *dataBuf) override
	{
		auto handle = m_device->Open(m_path, true);
		m_device->Read(handle, dataBuf, m_device->GetLength(handle));
		m_device->Close(handle);
	}
};

int Main::Run(const ProgramArguments& arguments)
{
	streaming::StreamMan manager(4);
	TestInterface dummyInterface;

	VfsResourceProvider resourceProvider("C:\\windows\\system32\\license.rtf");

	manager.RegisterResourceType(0, 500, &dummyInterface);
	manager.LinkResource(0, "dummy.txt", &resourceProvider);

	manager.Request(0);
	manager.LoadingBarrier();
	manager.UnlinkResource(0); // why does this not unload the resource?

	unknown();
	unknown();
	unknown();

	printf("hi! %s\n", arguments[2].c_str());

	return 0;
}
}