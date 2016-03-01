#include "StdInc.h"
#include "Entity.h"

#include "Game.h"

namespace krt
{

Entity::Entity(Game* ourGame)
{
	this->ourGame = ourGame;

	LIST_INSERT(ourGame->activeEntities.root, this->gameNode);

	this->modelID = -1;

	this->interiorId = 0;

	this->matrix.setIdentity();
	this->hasLocalMatrix = false;

	// Initialize the flags.
	this->isUnderwater            = false;
	this->isTunnelObject          = false;
	this->isTunnelTransition      = false;
	this->isUnimportantToStreamer = false;

	// Initialize LOD things.
	this->higherQualityEntity = NULL;
	this->lowerQualityEntity  = NULL;

	this->rwObject = NULL;

	this->onWorld = NULL; // we are not part of a world.

	this->cachedBoundSphereRadius = 400.0f; // TODO: cache the real bounding sphere radius in some file so we dont need the model

	this->lodChildrenCount = 0;
	this->lodChildrenDrawn = 0;
}

Entity::~Entity(void)
{
	// Make sure we released our RW object.
	this->DeleteRWObject();

	// Make sure we are not references anywhere anymore.
	{
		this->RemoveEntityFromWorldSectors();
	}

	// Remove us from any world.
	this->LinkToWorld(NULL);

	// Check whether we are linked as LOD or have a LOD.
	// Unlink those relationships.
	{
		if (Entity* highLOD = this->higherQualityEntity)
		{
			highLOD->lowerQualityEntity = NULL;

			this->higherQualityEntity = NULL;
		}

		if (Entity* lowLOD = this->lowerQualityEntity)
		{
			lowLOD->higherQualityEntity = NULL;

			this->lowerQualityEntity = NULL;
		}
	}

	// Remove us from the game.
	LIST_REMOVE(this->gameNode);
}

void Entity::SetModelIndex(streaming::ident_t modelID)
{
	// Make sure we have no RW object when switching models
	// This is required because models are not entirely thread-safe if not following certain rules.
	assert(this->rwObject == NULL);

	this->modelID = modelID;
}

static rw::Frame* RwObjectGetFrame(rw::Object* rwObj)
{
	rw::uint8 objType = rwObj->type;

	if (objType == rw::Atomic::ID)
	{
		rw::Atomic* atomic = (rw::Atomic*)rwObj;

		return atomic->getFrame();
	}
	else if (objType == rw::Clump::ID)
	{
		rw::Clump* clump = (rw::Clump*)rwObj;

		return clump->getFrame();
	}

	return NULL;
}

bool Entity::CreateRWObject(void)
{
	streaming::ident_t modelID = this->modelID;

	if (modelID == -1)
		return false;

	// If we already have an atomic, we refuse to update.
	if (this->rwObject)
		return false;

	// Fetch a model from the model manager.
	Game* game = theGame;

	ModelManager::ModelResource* modelEntry = game->GetModelManager().GetModelByID(modelID);

	if (!modelEntry)
		return false;

	rw::Object* rwobj = modelEntry->CloneModel();

	if (!rwobj)
		return false;

	// Make sure our RW object has a frame.
	// This is because we will render it.
	{
		rw::uint8 objType = rwobj->type;

		if (objType == rw::Atomic::ID)
		{
			rw::Atomic* atomic = (rw::Atomic*)rwobj;

			if (atomic->getFrame() == NULL)
			{
				rw::Frame* parentFrame = rw::Frame::create();

				atomic->setFrame(parentFrame);
			}
		}
		else if (objType == rw::Clump::ID)
		{
			rw::Clump* clump = (rw::Clump*)rwobj;

			if (clump->getFrame() == NULL)
			{
				rw::Frame* parentFrame = rw::Frame::create();

				clump->setFrame(parentFrame);
			}
		}
	}

	this->rwObject = rwobj;

	// If we have a local matrix, then set it into our frame.
	{
		if (this->hasLocalMatrix)
		{
			rw::Frame* parentFrame = RwObjectGetFrame(rwobj);

			if (parentFrame)
			{
				parentFrame->matrix = this->matrix;
				parentFrame->updateObjects();

				this->hasLocalMatrix = false;
			}
		}
	}

	return true;
}

void Entity::DeleteRWObject(void)
{
	rw::Object* rwobj = this->rwObject;

	if (!rwobj)
		return;

	Game* game = theGame;

	ModelManager::ModelResource* modelEntry = game->GetModelManager().GetModelByID(this->modelID);

	if (!modelEntry)
		return;

	// Store our matrix.
	{
		rw::Frame* parentFrame = RwObjectGetFrame(rwobj);

		if (parentFrame)
		{
			this->matrix = parentFrame->matrix;

			this->hasLocalMatrix = true;
		}
	}

	modelEntry->ReleaseModel(rwobj);

	this->rwObject = NULL;
}

void Entity::SetModelling(const rw::Matrix& mat)
{
	rw::Object* rwobj = this->rwObject;

	if (rwobj == NULL)
	{
		this->matrix = mat;

		this->hasLocalMatrix = true;
	}
	else
	{
		rw::Frame* parentFrame = RwObjectGetFrame(rwobj);

		assert(parentFrame != NULL);

		if (parentFrame)
		{
			parentFrame->matrix = mat;
			parentFrame->updateObjects(); // that's kinda how RW works; you say that you updated the struct.
		}
	}
}

const rw::Matrix& Entity::GetModelling(void) const
{
	rw::Object* rwobj = this->rwObject;

	if (rwobj)
	{
		rw::Frame* parentFrame = RwObjectGetFrame(rwobj);

		if (parentFrame)
		{
			return parentFrame->matrix;
		}
	}

	return this->matrix;
}

const rw::Matrix& Entity::GetMatrix(void) const
{
	rw::Object* rwobj = this->rwObject;

	if (rwobj)
	{
		rw::Frame* parentFrame = RwObjectGetFrame(rwobj);

		if (parentFrame)
		{
			return *parentFrame->getLTM();
		}
	}

	return this->matrix;
}

static bool RpClumpCalculateBoundingSphere(rw::Clump* clump, rw::Sphere& sphereOut)
{
	rw::Sphere tmpSphere;
	bool hasSphere = false;

	rw::clumpForAllAtomics(clump,
	    [&](rw::Atomic* atomic) {
		    rw::Sphere* atomicSphere = atomic->getWorldBoundingSphere();

		    if (atomicSphere)
		    {
			    if (!hasSphere)
			    {
				    tmpSphere = *atomicSphere;

				    hasSphere = true;
			    }
			    else
			    {
				    // Create a new sphere that encloses both spheres.
				    rw::V3d vecHalfDist = rw::scale(rw::sub(atomicSphere->center, tmpSphere.center), 0.5f);

				    // Adjust the radius.
				    float vecHalfDistScalar = rw::length(vecHalfDist);

				    tmpSphere.center = rw::add(tmpSphere.center, vecHalfDist);
				    tmpSphere.radius = std::max(tmpSphere.radius, atomicSphere->radius) + vecHalfDistScalar;
			    }
		    }
		});

	if (!hasSphere)
		return false;

	sphereOut = tmpSphere;
	return true;
}

bool Entity::GetWorldBoundingSphere(rw::Sphere& sphereOut) const
{
	rw::Object* rwObj = this->rwObject;

	if (rwObj == NULL)
	{
		// Even if we have no model we need a bounding sphere.
		const rw::Matrix& curMatrix = this->GetMatrix();

		sphereOut.center = curMatrix.pos;
		sphereOut.radius = this->cachedBoundSphereRadius;

		return true;
	}

	bool hasSphere = false;

	if (rwObj->type == rw::Atomic::ID)
	{
		rw::Atomic* atomic = (rw::Atomic*)rwObj;

		rw::Sphere* atomicSphere = atomic->getWorldBoundingSphere();

		if (atomicSphere)
		{
			sphereOut = *atomicSphere;

			hasSphere = true;
		}
	}
	else if (rwObj->type == rw::Clump::ID)
	{
		rw::Clump* clump = (rw::Clump*)rwObj;

		hasSphere = RpClumpCalculateBoundingSphere(clump, sphereOut);
	}

	if (hasSphere)
	{
		// Store the last calculated bounding sphere radius for later.
		this->cachedBoundSphereRadius = sphereOut.radius;
	}

	return hasSphere;
}

void Entity::LinkToWorld(World* theWorld)
{
	if (World* prevWorld = this->onWorld)
	{
		LIST_REMOVE(this->worldNode);

		this->onWorld = NULL;
	}

	if (theWorld)
	{
		LIST_INSERT(theWorld->entityList.root, worldNode);

		this->onWorld = theWorld;
	}
}

World* Entity::GetWorld(void)
{
	return onWorld;
}

void Entity::SetLODEntity(Entity* lodInst)
{
	if (Entity* prevLOD = this->lowerQualityEntity)
	{
		prevLOD->higherQualityEntity = NULL;

		this->lowerQualityEntity = NULL;
	}

	if (lodInst)
	{
		if (Entity* prevHighLOD = lodInst->higherQualityEntity)
		{
			prevHighLOD->lowerQualityEntity = NULL;
		}

		this->lowerQualityEntity = lodInst;

		lodInst->lodChildrenCount++;
		lodInst->higherQualityEntity = this;
	}
}

Entity* Entity::GetLODEntity(void)
{
	return this->lowerQualityEntity;
}

bool Entity::IsLowerLODOf(Entity* inst) const
{
	Entity* entity = this->lowerQualityEntity;

	while (entity)
	{
		if (entity == inst)
			return true;

		entity = entity->lowerQualityEntity;
	}

	return false;
}

bool Entity::IsHigherLODOf(Entity* inst) const
{
	Entity* entity = this->higherQualityEntity;

	while (entity)
	{
		if (entity == inst)
			return true;

		entity = entity->higherQualityEntity;
	}

	return false;
}

ModelManager::ModelResource* Entity::GetModelInfo(void) const
{
	return theGame->GetModelManager().GetModelByID(this->modelID);
}

void Entity::AddEntityWorldSectorReference(EntityReference* refPtr)
{
	this->worldSectorReferences.push_back(refPtr);
}

void Entity::RemoveEntityFromWorldSectors(void)
{
	while (this->worldSectorReferences.empty() == false)
	{
		EntityReference* refPtr = this->worldSectorReferences.front();

		refPtr->Unlink();
	}
}

void Entity::RemoveEntityWorldReference(EntityReference* refPtr)
{
	auto find_iter = std::find(this->worldSectorReferences.begin(), this->worldSectorReferences.end(), refPtr);

	if (find_iter == this->worldSectorReferences.end())
		return;

	this->worldSectorReferences.erase(find_iter);
}
}