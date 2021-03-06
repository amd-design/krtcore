#pragma once

// Management for model resources in our engine!

#include <Streaming.common.h>

#include "TexDict.h"

#include <Console.CommandHelpers.h>

// custom include guard
#ifndef COLLISION_H
#define COLLISION_H

#include "src/collision.h"
#endif

#define MODEL_ID_BASE 0
#define MAX_MODELS 20000

namespace krt
{

// General model information management container.
struct ModelManager : public streaming::StreamingTypeInterface
{
	enum class eModelType
	{
		ATOMIC,
		VEHICLE,
		PED
	};

	struct ModelResource
	{
		inline streaming::ident_t GetID(void) const { return this->id; }
		inline eModelType GetType(void) const { return this->modelType; }
		inline float GetLODDistance(void) const { return this->lodDistance; }
		inline int GetFlags(void) const { return this->flags; }

		inline ModelResource* GetLODModel(void) { return this->lod_model; }

		inline std::shared_ptr<CColModel> GetCollisionModel()
		{
			if (this->col_model.expired())
			{
				return nullptr;
			}

			return this->col_model.lock();
		}

		inline void SetCollisionModel(const std::shared_ptr<CColModel>& pointer)
		{
			this->col_model = pointer;
		}

		rw::Object* CloneModel(void);
		void ReleaseModel(rw::Object* rwobj);

		inline float GetMinimumDistance(void) const
		{
			// if not a LOD, no minimum distance
			if (this->lodDistance < 300.0f)
			{
				return 0.0f;
			}

			// if we have a non-LOD model, don't draw once we draw that
			if (this->non_lod_model)
			{
				return this->non_lod_model->GetLODDistance();
			}

			// use the game-specific minimum distance
			return this->minimumDistance;
		}

	private:
		friend struct ModelManager;

		static void NativeReleaseModel(rw::Object* rwobj);

		inline ModelResource(vfs::DevicePtr device, std::string pathToRes) : vfsResLoc(device, std::move(pathToRes))
		{
			return;
		}

		inline ~ModelResource(void)
		{
			return;
		}

		ModelManager* manager;

		streaming::ident_t id;
		streaming::ident_t texDictID;
		float lodDistance;
		float minimumDistance;
		int flags;

		ModelResource* non_lod_model; // non-SA: model of a higher-quality model than this one
		ModelResource* lod_model;     // model of a lower quality model than this one.

		std::weak_ptr<CColModel> col_model;

		eModelType modelType;

		DeviceResourceLocation vfsResLoc;

		rw::Object* modelPtr;

		SRWLOCK_VIRTUAL lockModelLoading;
	};

	ModelManager(streaming::StreamMan& streaming, TextureManager& texManager);
	~ModelManager(void);

	streaming::ident_t RegisterAtomicModel(
	    std::string name, std::string texDictName, float lodDistance, int flags,
	    std::string absFilePath);

	ModelResource* GetModelByID(streaming::ident_t id);

	ModelResource* GetModelByName(const std::string& name);

	void LoadAllModels(void);

	void LoadResource(streaming::ident_t localID, const void* dataBuf, size_t memSize) override;
	void UnloadResource(streaming::ident_t localID) override;

	size_t GetObjectMemorySize(streaming::ident_t localID) const override;

private:
	streaming::StreamMan& streaming;
	TextureManager& texManager;

	std::vector<ModelResource*> models;

	std::atomic<streaming::ident_t> curModelId;

	std::map<std::string, ModelResource*> modelByName;

	std::map<std::string, ModelResource*> basifierLookup;
	std::map<std::string, ModelResource*> lodifierLookup;

	std::unique_ptr<ConsoleCommand> loadAllModelsCommand;
};
}