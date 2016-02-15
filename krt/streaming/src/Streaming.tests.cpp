#include "StdInc.h"
#include "Streaming.h"
#include "vfs\Manager.h"
#include "CdImageDevice.h"

#include <iostream>

namespace krt
{

namespace streaming
{

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

static std::mutex safety_lock;

static void DebugMessage(const std::string& msg)
{
	std::unique_lock<std::mutex> msgLock;

	std::cout << msg << std::endl;
}

class TestInterface : public streaming::StreamingTypeInterface
{
  public:
	virtual void LoadResource(streaming::ident_t localID, const void* data, size_t dataSize) override
	{
		const RwSectionHeader* header =
		    reinterpret_cast<const RwSectionHeader*>(data);

		DebugMessage(std::to_string(localID) + ": type " +
		             std::to_string(header->type) + ", size " +
		             std::to_string(header->size) + ", version " +
		             std::to_string(libraryIDUnpackVersion(header->libid)));
	}
	virtual void UnloadResource(streaming::ident_t localID) override
	{
		DebugMessage(std::to_string(localID) + "-");
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
		m_path   = path;
		m_device = vfs::GetDevice(path);
		m_length = m_device->GetLength(m_path);
	}

	virtual size_t getDataSize(void) const override
	{
		return m_length;
	}

	virtual void fetchData(void* dataBuf) override
	{
		uint64_t ptr;
		auto handle = m_device->OpenBulk(m_path, &ptr);
		m_device->ReadBulk(handle, ptr, dataBuf, m_length);
		m_device->CloseBulk(handle);
	}
};

class CdImageResourceEntry : public streaming::ResourceLocation
{
  private:
	std::string m_path;
	vfs::DevicePtr m_device;

	size_t m_length;

  public:
	CdImageResourceEntry(vfs::DevicePtr device, std::string path)
	{
		m_device = device;
		m_length = device->GetLength(path);
		m_path   = std::move(path);

		assert(m_length != -1);
	}

	size_t getDataSize(void) const override
	{
		return m_length;
	}

	void fetchData(void* dataBuf) override
	{
		uint64_t ptr;
		auto handle = m_device->OpenBulk(m_path, &ptr);
		m_device->ReadBulk(handle, ptr, dataBuf, m_length);
		m_device->CloseBulk(handle);
	}
};

void Test1( void )
{
	std::shared_ptr<streaming::CdImageDevice> cdImage = std::make_shared<streaming::CdImageDevice>();

	// Load the game image to start loading things.
	{
		const char* computerName = getenv("COMPUTERNAME");

		if (!strcmp(computerName, "FALLARBOR"))
		{
			bool loadResult = cdImage->OpenImage("S:\\Games\\Steam\\steamapps\\common\\Grand Theft Auto 3\\models\\gta3.img");

			assert(loadResult);
		}
		else if (strcmp(computerName, "DESKTOP") == 0)
		{
			bool loadResult = cdImage->OpenImage("D:\\gtaiso\\unpack\\gtavc\\MODELS\\gta3.img");

			assert(loadResult == true);
		}
	}

	// We can only run our beautiful test code if you play along :(
	assert(cdImage != nullptr);

	vfs::Mount(cdImage, "gta3img:/");

	streaming::StreamMan manager(4);
	TestInterface dummyInterface;

	manager.RegisterResourceType(0, 20000, &dummyInterface);

	// Link all DFF resources.
	streaming::ident_t availID = 0;

	std::list<CdImageResourceEntry> modelResources;
	{
		vfs::FindData findData;

		streaming::CdImageDevice::THandle findHandle = cdImage->FindFirst("gta3img:/", &findData);

		if (findHandle != streaming::CdImageDevice::InvalidHandle)
		{
			// Look through it, meow.
			do
			{
				// Why do I have to do this? :/
				std::string realPathName = "gta3img:/" + findData.name;

				modelResources.push_back(CdImageResourceEntry(cdImage, realPathName));

				manager.LinkResource(availID++, realPathName, &modelResources.back());
			} while (cdImage->FindNext(findHandle, &findData));

			cdImage->FindClose(findHandle);
		}
	}

	// Request everything in steps of ten!
	const streaming::ident_t loaderStepCount = 10;
	streaming::ident_t resLoadIndex          = 0;

	const streaming::ident_t numToLoad = availID;

	while (resLoadIndex < numToLoad)
	{
		for (streaming::ident_t n = 0; n < loaderStepCount; n++)
		{
			const streaming::ident_t curID = (resLoadIndex++);

			if (curID < numToLoad)
			{
				manager.Request(curID);
			}
		}
	}

    // Wait for some random resource in the middle to be loaded.
    manager.WaitForResource( 2500 );

	// Check for loaded resource :)
	{
		streaming::StreamingStats stats;

		manager.GetStatistics(stats);

		assert(stats.memoryInUse != 0);
	}

    // Lets try unloading things while they are loading!!!
    resLoadIndex = 0;

    while ( resLoadIndex < numToLoad )
    {
        manager.UnlinkResource( resLoadIndex++ );
    }

    manager.LoadingBarrier();
}

}

}