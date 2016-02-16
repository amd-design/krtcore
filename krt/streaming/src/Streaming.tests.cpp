#include "StdInc.h"
#include "Streaming.h"
#include "vfs\Manager.h"
#include "CdImageDevice.h"

#include "Streaming.common.h"

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

inline std::shared_ptr<streaming::CdImageDevice> GetTestingCdImage( void )
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

    return cdImage;
}

struct MountAndReadyImage
{
    inline MountAndReadyImage( void ) : manager( 4 )
    {
        cdImage = GetTestingCdImage();

	    // We can only run our beautiful test code if you play along :(
	    assert(cdImage != nullptr);

	    vfs::Mount(cdImage, "gta3img:/");

	    manager.RegisterResourceType(0, 20000, &dummyInterface);

	    // Link all DFF resources.
	    streaming::ident_t availID = 0;
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

				    modelResources.push_back(DeviceResourceLocation(cdImage, realPathName));

				    manager.LinkResource(availID++, realPathName, &modelResources.back());
			    } while (cdImage->FindNext(findHandle, &findData));

			    cdImage->FindClose(findHandle);
		    }
	    }

        numResources = availID;
    }

	TestInterface dummyInterface;

    vfs::DevicePtr cdImage;
    
    ident_t numResources;

    std::list<DeviceResourceLocation> modelResources;

	streaming::StreamMan manager;
};

void Test1( void )
{
    MountAndReadyImage imgDesc;

    StreamMan& manager = imgDesc.manager;

    auto cdImage = imgDesc.cdImage;

	// Request everything in steps of ten!
	const streaming::ident_t loaderStepCount = 10;
	streaming::ident_t resLoadIndex          = 0;

	const streaming::ident_t numToLoad = imgDesc.numResources;

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

void Test2( void )
{
    MountAndReadyImage imgDesc;

    StreamMan& manager = imgDesc.manager;

    auto cdImage = imgDesc.cdImage;

    // Do some dependency loading.
    const streaming::ident_t dependSteps = 100;
    const streaming::ident_t resToLoad = imgDesc.numResources;

    streaming::ident_t iter = 0;

    while ( iter < resToLoad )
    {
        // We make a lot of resources depend on some resources.
        ident_t dependOn = iter++;

        for ( ident_t sub_iter = 0; sub_iter < dependSteps; sub_iter++ )
        {
            manager.AddResourceDependency( dependOn, iter++ );
        }

        iter += ( dependSteps + 1 );
    }

    // Request a lot of resources :)
    iter = 0;

    while ( iter < resToLoad )
    {
        manager.Request( iter++ );
    }

    manager.Unload( 1337 );

    manager.LoadingBarrier();
}

void Test3( void )
{
    MountAndReadyImage imgDesc;

    StreamMan& manager = imgDesc.manager;

    auto cdImage = imgDesc.cdImage;

    // Do some dependency loading.
    const streaming::ident_t dependSteps = 100;
    const streaming::ident_t resToLoad = imgDesc.numResources;

    streaming::ident_t iter = 0;

    while ( iter < resToLoad )
    {
        // We make a lot of resources depend on some resources.
        ident_t dependOn = iter++;

        for ( ident_t sub_iter = 0; sub_iter < dependSteps; sub_iter++ )
        {
            manager.AddResourceDependency( dependOn, iter++ );
        }

        iter += ( dependSteps + 1 );
    }

    manager.Request( 0 );

    manager.LoadingBarrier();

    // Try to unload some resources that really should not unload.
    manager.Unload( 11 );
    manager.Unload( 23 );

    manager.LoadingBarrier();

    // They have to stay loaded!
    assert( manager.GetResourceStatus( 11 ) == StreamMan::eResourceStatus::LOADED );
    assert( manager.GetResourceStatus( 23 ) == StreamMan::eResourceStatus::LOADED );

    // Unlink some stuff I guess.
    manager.UnlinkResource( 49 );
    manager.UnlinkResource( 0 );
}

// Test certain resource locations that fail a certain state, on purpose.
// We want to be able to fail any stage of the streaming system, because resources can contain errors.
struct ResLocFailSize : public streaming::ResourceLocation
{
    size_t getDataSize( void ) const override
    {
        throw std::exception( "nasty bug" );
    }

    void fetchData( void *dataBuf ) override
    {
        // meow.
    }
};

struct ResLocFailFetch : public streaming::ResourceLocation
{
    size_t getDataSize( void ) const override
    {
        return some_text.size();
    }

    void fetchData( void *dataBuf ) override
    {
        throw std::exception( "nasty bug the second" );
    }

    std::string some_text = "Hello world!";
};

struct ResLocCoolio : public streaming::ResourceLocation
{
    size_t getDataSize( void ) const override
    {
        return some_text.size();
    }

    void fetchData( void *dataBuf ) override
    {
        memcpy( dataBuf, some_text.c_str(), some_text.size() );
    }

    std::string some_text = "Hello world!";
};

// Some faulty data loaders.
struct StreamTypeMrFaulty : public streaming::StreamingTypeInterface
{
    void LoadResource( ident_t localID, const void *data, size_t dataSize ) override
    {
        throw std::exception( "meow this is a bad programming error." );
    }

    void UnloadResource( ident_t localID ) override
    {
        //meow.
    }

    size_t GetObjectMemorySize( ident_t localID ) const override
    {
        return 0;
    }
};

struct StreamTypeUnloadFaulty : public streaming::StreamingTypeInterface
{
    void LoadResource( ident_t localID, const void *data, size_t dataSize ) override
    {
        //meow.
    }

    void UnloadResource( ident_t localID ) override
    {
        throw std::exception( "oh no something horrible happened!" );
    }

    size_t GetObjectMemorySize( ident_t localID ) const override
    {
        return 0;
    }
};

// Test made in mind to raise as many errors as possible.
void FaultTest1( void )
{
    auto cdImage = GetTestingCdImage();

	// We can only run our beautiful test code if you play along :(
	assert(cdImage != nullptr);

	vfs::Mount(cdImage, "gta3img:/");

    StreamMan manager( 1 );

    // Test exception safety.
    {
        // getDataSize.
        {
            ResLocFailSize resLoc;
            StreamTypeMrFaulty streamType;

            manager.RegisterResourceType( 0, 1, &streamType );

            manager.LinkResource( 0, "getDataSize-fault", &resLoc );

            manager.Request( 0 );
            manager.LoadingBarrier();

            manager.UnlinkResource( 0 );
            manager.UnregisterResourceType( 0 );
        }

        // fetchData.
        {
            ResLocFailFetch resLoc;
            StreamTypeMrFaulty streamType;

            manager.RegisterResourceType( 0, 1, &streamType );

            manager.LinkResource( 0, "fetchData-fault", &resLoc );

            manager.Request( 0 );
            manager.LoadingBarrier();

            manager.UnlinkResource( 0 );
            manager.UnregisterResourceType( 0 );
        }

        // LoadResource.
        {
            ResLocCoolio resLoc;
            StreamTypeMrFaulty streamType;

            manager.RegisterResourceType( 0, 1, &streamType );

            manager.LinkResource( 0, "LoadResource-fault", &resLoc );

            manager.Request( 0 );
            manager.LoadingBarrier();

            manager.UnlinkResource( 0 );
            manager.UnregisterResourceType( 0 );
        }

        // UnloadResource.
        {
            ResLocCoolio resLoc;
            StreamTypeUnloadFaulty streamType;

            manager.RegisterResourceType( 0, 1, &streamType );

            manager.LinkResource( 0, "UnloadResource-fault", &resLoc );

            manager.Request( 0 );
            manager.LoadingBarrier();

            manager.UnlinkResource( 0 );
            manager.UnregisterResourceType( 0 );
        }

        // UnloadResource with destructor failure.
        {
            ResLocCoolio resLoc;
            StreamTypeUnloadFaulty streamType;

            manager.RegisterResourceType( 0, 1, &streamType );

            manager.LinkResource( 0, "UnloadResource-fault", &resLoc );

            manager.Request( 0 );
            manager.LoadingBarrier();
        }
    }
}

}

}