#pragma once

// World class for managing all alive Entities.
#include "Entity.h"

#include "World.SectorGrid.h"

namespace krt
{

struct World
{
	friend struct Entity;

	World(void);
	~World(void);

	void DepopulateEntities(void);
	void PutEntitiesOnGrid(void);

	void RenderWorld(void* gpuDevice);

private:
	NestedList<Entity> entityList;

	// World sectors for optimized entity rendering.
	struct StaticEntitySector
	{
		inline StaticEntitySector(void)
		{
		}

		void Clear(void)
		{
			return;
		}

		void AddEntity(Entity* theEntity)
		{
			this->entitiesOnSector.push_back(theEntity);
		}

		void RemoveEntity(Entity* theEntity)
		{
			auto findIter = std::find(entitiesOnSector.begin(), entitiesOnSector.end(), theEntity);

			if (findIter != entitiesOnSector.end())
			{
				entitiesOnSector.erase(findIter);
			}
		}

		std::list<Entity*> entitiesOnSector;
	};

	SectorGrid<StaticEntitySector, 3000, 3> staticEntityGrid;
};
};