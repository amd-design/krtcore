#include "StdInc.h"
#include "RTMain.h"
#include "ProgramArguments.h"

#include <stdio.h>

#include <vfs/Manager.h>
#include <Streaming.h>
#include <CdImageDevice.h>

namespace krt
{
int unknown()
{
	vfs::StreamPtr stream = vfs::OpenRead("C:\\Windows\\system32\\License.rtf");
	auto data = stream->ReadToEnd();

	assert(false && "hey!");

	return 0;
}

struct RwSectionHeader
{
	uint32_t type;
	uint32_t size;
	uint32_t libid;
};

// by aap
inline int
	libraryIDUnpackVersion(uint32_t libid)
{
	if (libid & 0xFFFF0000)
		return (libid >> 14 & 0x3FF00) + 0x30000 |
		(libid >> 16 & 0x3F);
	else
		return libid << 8;
}

class TestInterface : public streaming::StreamingTypeInterface
{
public:
	virtual void LoadResource(streaming::ident_t localID, const void *data, size_t dataSize) override
	{
		const RwSectionHeader* header = reinterpret_cast<const RwSectionHeader*>(data);

		printf("%d: type %d, size %d, version %x\n", localID, header->type, header->size, libraryIDUnpackVersion(header->libid));
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

	size_t m_length;

public:
	VfsResourceProvider(const std::string& path)
	{
		m_path = path;
		m_device = vfs::GetDevice(path);
		m_length = m_device->GetLength(m_path);
	}

	virtual size_t getDataSize(void) const override
	{
		return m_length;
	}

	virtual void fetchData(void *dataBuf) override
	{
		uint64_t ptr;
		auto handle = m_device->OpenBulk(m_path, &ptr);
		m_device->ReadBulk(handle, ptr, dataBuf, m_length);
		m_device->CloseBulk(handle);
	}
};

int Main::Run(const ProgramArguments& arguments)
{
	std::shared_ptr<streaming::CdImageDevice> cdImage = std::make_shared<streaming::CdImageDevice>();

	if (!strcmp(getenv("COMPUTERNAME"), "FALLARBOR"))
	{
		bool loadResult = cdImage->OpenImage("S:\\Games\\Steam\\steamapps\\common\\Grand Theft Auto 3\\models\\gta3.img");

		assert(loadResult);
	}

	vfs::Mount(cdImage, "gta3img:/");

	streaming::StreamMan manager(4);
	TestInterface dummyInterface;

	VfsResourceProvider resourceProvider("gta3img:/male01.dff");

	manager.RegisterResourceType(0, 500, &dummyInterface);
	manager.LinkResource(0, "male01.dff", &resourceProvider);

	manager.Request(0);
	manager.LoadingBarrier();

    // Check for loaded resource :)
    {
        streaming::StreamingStats stats;

        manager.GetStatistics( stats );

        assert( stats.memoryInUse != 0 );
    }

	manager.UnlinkResource(0); // why does this not unload the resource?

    // Now check again.
    {
        streaming::StreamingStats stats;

        manager.GetStatistics( stats );

        assert( stats.memoryInUse == 0 );
    }

	unknown();
	unknown();
	unknown();

	printf("hi! %s\n", arguments[2].c_str());

	return 0;
}
}