#pragma once

// Texture Dictionary storage system.

#include "Streaming.common.h"

#define TXD_START_ID 20000
#define MAX_TXD 5000

namespace krt
{

struct TextureManager : public streaming::StreamingTypeInterface
{
	TextureManager(streaming::StreamMan& streaming);
	~TextureManager(void);

	void RegisterResource(std::string name, vfs::DevicePtr device, std::string pathToRes);

	streaming::ident_t FindTexDict(const std::string& name) const;
	void SetTexParent(const std::string& texName, const std::string& texParentName);

	void SetCurrentTXD(streaming::ident_t id);
	void UnsetCurrentTXD(void);

	void LoadResource(streaming::ident_t localID, const void* data, size_t dataSize) override;
	void UnloadResource(streaming::ident_t localID) override;

	size_t GetObjectMemorySize(streaming::ident_t localID) const override;

private:
	streaming::StreamMan& streaming;

	static rw::Texture* _rwFindTextureCallback(const char* name);

	rw::Texture* (*_tmpStoreOldCB)(const char* name);

	struct TexDictResource
	{
		TexDictResource(vfs::DevicePtr device, std::string pathToRes) : vfsResLoc(device, pathToRes)
		{
			return;
		}

		~TexDictResource(void)
		{
			return;
		}

		streaming::ident_t id;
		streaming::ident_t parentID;

		DeviceResourceLocation vfsResLoc;

		rw::TexDictionary* txdPtr;

		SRWLOCK_VIRTUAL lockResourceLoad;
	};

	struct ThreadLocal_CurrentTXDEnv
	{
		inline ThreadLocal_CurrentTXDEnv(TextureManager* manager)
		{
			exclusive_lock_acquire<std::shared_timed_mutex> ctxAddTXDEnv(manager->lockTXDEnvList);

			LIST_INSERT(manager->_tlCurrentTXDEnvList.root, node);

			this->isRegistered = true;
			this->manager      = manager;
		}

		inline ~ThreadLocal_CurrentTXDEnv(void)
		{
			if (manager != NULL)
			{
				exclusive_lock_acquire<std::shared_timed_mutex> ctxRemoveTXDEnv(manager->lockTXDEnvList);

				if (this->isRegistered)
				{
					LIST_REMOVE(node);

					this->isRegistered = false;
				}
			}
		}

		rw::TexDictionary* currentTXD;

		bool isRegistered;
		NestedListEntry<ThreadLocal_CurrentTXDEnv> node;

		TextureManager* manager;
	};

	NestedList<ThreadLocal_CurrentTXDEnv> _tlCurrentTXDEnvList;

	mutable std::shared_timed_mutex lockTXDEnvList;

	ThreadLocal_CurrentTXDEnv* GetCurrentTXDEnv(void);

	TexDictResource* FindTexDictInternal(const std::string& name) const;

	std::vector<TexDictResource*> texDictList;

	std::map<std::string, TexDictResource*> texDictMap;
};
}