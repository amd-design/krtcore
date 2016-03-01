#include "StdInc.h"
#include "CollisionStore.h"
#include "ModelInfo.h"
#include "Game.h"

#include <vfs/Manager.h>

#define COL_ID_BASE 25000
#define MAX_COLS 1000

namespace krt
{
CollisionStore::CollisionStore(streaming::StreamMan& streaming) : m_streaming(streaming), m_curIndex(0)
{
	bool didRegister = streaming.RegisterResourceType(COL_ID_BASE, MAX_COLS, this);
	assert(didRegister == true);

	m_entries.resize(MAX_COLS);
}

CollisionStore::~CollisionStore()
{
	// remove all entries
	for (auto& entry : m_entries)
	{
		if (entry.get())
		{
			m_streaming.UnlinkResource(entry->GetID());
		}
	}

	// clear the entry list and unregister ourselves
	m_entries.clear();

	m_streaming.UnregisterResourceType(COL_ID_BASE);
}

streaming::ident_t CollisionStore::RegisterCollisionArchive(const std::string& name, const std::string& filePath)
{
	// get an identifier
	streaming::ident_t id = (m_curIndex.fetch_add(1)) + COL_ID_BASE;

	// get a device
	vfs::DevicePtr resDevice = vfs::GetDevice(filePath);

	if (resDevice == nullptr)
	{
		// no device? no entry
		return -1;
	}

	// check whether this resource even exists
	if (resDevice->GetLength(filePath) == -1)
	{
		// non-existent entry
		return -1;
	}

	// create a new archive
	auto archive = std::make_shared<CollisionArchive>(resDevice, filePath);
	archive->m_id = id;
	archive->m_store = this;

	// register with streaming
	if (!m_streaming.LinkResource(id, name, &archive->m_location))
	{
		// registration failed
		return -1;
	}

	// store us
	m_entries[id - COL_ID_BASE] = archive;

	return id;
}

enum { COLL = 0x4C4C4F43 };

void CollisionStore::LoadResource(streaming::ident_t localID, const void* dataBuf, size_t memSize)
{
	struct
	{
		uint32_t ident;
		uint32_t size;
	} header;

	vfs::StreamPtr stream = vfs::OpenRead(vfs::MakeMemoryFilename(dataBuf, memSize));

	auto archive = m_entries[localID];
	
	while (true)
	{
		// read the header
		if (stream->Read(&header, sizeof(header)) != sizeof(header))
		{
			break;
		}

		// does the identification match?
		if (header.ident != COLL)
		{
			break;
		}

		// read the collision data
		std::vector<uint8_t> data = stream->Read(header.size);

		// read the name
		char name[24];

		memcpy(name, &data[0], sizeof(name));

		// read the collision model
		std::shared_ptr<CColModel> model = std::make_shared<CColModel>();
		readColModel(model.get(), &data[24]);

		// get the model info
		ModelManager::ModelResource* modelResource = theGame->GetModelManager().GetModelByName(name);

		// store data
		std::unique_ptr<CollisionModel> colModel = std::make_unique<CollisionModel>();
		colModel->name = name;
		colModel->model = model;

		if (modelResource)
		{
			colModel->modelIndex = modelResource->GetID();
			modelResource->SetCollisionModel(model);
		}
		else
		{
			colModel->modelIndex = -1;
		}

		archive->m_models.push_back(std::move(colModel));
	}
}

void CollisionStore::UnloadResource(streaming::ident_t localID)
{
	std::shared_ptr<CollisionArchive> archive = m_entries[localID];

	// this should dereference everything inside
	archive->m_models.clear();
}

size_t CollisionStore::GetObjectMemorySize(streaming::ident_t localID) const
{
	return 1;
}

ConsoleCommand loadCollCommand("load_coll", [] (const std::string& path)
{
	streaming::ident_t identifier = theGame->GetCollisionStore().RegisterCollisionArchive(path, path);
	
	if (identifier != -1)
	{
		theGame->GetStreaming().Request(identifier);
		theGame->GetStreaming().LoadingBarrier();
	}
});
}