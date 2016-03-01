#pragma once

#include <Streaming.common.h>

#ifndef COLLISION_H
#define COLLISION_H

#include "src/collision.h"
#endif

namespace krt
{
class CollisionStore : public streaming::StreamingTypeInterface
{
public:
	struct CollisionModel
	{
		streaming::ident_t modelIndex;
		std::string name;
		std::shared_ptr<CColModel> model;
	};

	class CollisionArchive
	{
	public:
		inline streaming::ident_t GetID() const
		{
			return m_id;
		}

		inline const std::vector<std::unique_ptr<CollisionModel>>& GetModels() const
		{
			return m_models;
		}

		inline CollisionArchive(const vfs::DevicePtr& device, const std::string& pathToRes) : m_location(device, pathToRes)
		{
		}

	private:
		friend class CollisionStore;

	private:
		CollisionStore* m_store;

		streaming::ident_t m_id;

		std::vector<std::unique_ptr<CollisionModel>> m_models;

		DeviceResourceLocation m_location;
	};

public:
	CollisionStore(streaming::StreamMan& streaming);
	~CollisionStore();

	streaming::ident_t RegisterCollisionArchive(const std::string& name, const std::string& filePath);

	virtual void LoadResource(streaming::ident_t localID, const void* dataBuf, size_t memSize) override;
	virtual void UnloadResource(streaming::ident_t localID) override;

	virtual size_t GetObjectMemorySize(streaming::ident_t localID) const override;

private:
	streaming::StreamMan& m_streaming;

	std::vector<std::shared_ptr<CollisionArchive>> m_entries;

	std::atomic<streaming::ident_t> m_curIndex;
};
}