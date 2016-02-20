#include <StdInc.h>
#include <FileLoader.h>

#include <Game.h>
#include <GameUniverse.h>

#include <vfs/Manager.h>
#include <vfs/Stream.h>

#include "CdImageDevice.h"

#include <Console.VariableHelpers.h>

namespace krt
{
static void make_lower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

inline std::string get_file_name(std::string path, std::string* extOut = NULL)
{
	const char* pathIter = path.c_str();

	const char* fileNameStart = pathIter;
	const char* fileExtEnd    = NULL;

	while (char c = *pathIter)
	{
		if (c == '\\' || c == '/')
		{
			fileNameStart = (pathIter + 1);
		}

		if (c == '.')
		{
			fileExtEnd = pathIter;
		}

		pathIter++;
	}

	if (fileExtEnd != NULL)
	{
		if (extOut)
		{
			*extOut = std::string(fileExtEnd + 1, pathIter);
		}

		return std::string(fileNameStart, fileExtEnd);
	}

	if (extOut)
	{
		extOut->clear();
	}

	return std::string(fileNameStart, pathIter);
}

static bool g_fileloaderDebug;

void FileLoader::ScanIMG(const vfs::DevicePtr& imgDevice, const std::string& pathPrefix, const GameUniversePtr& universe)
{
	if (g_fileloaderDebug)
	{
		console::Printf("Scanning CD image %s\n", pathPrefix.c_str());
	}

	// We now have to parse the contents of the IMG archive. :)
	{
		vfs::FindData findData;

		auto findHandle = imgDevice->FindFirst(pathPrefix, &findData);

		if (findHandle != vfs::Device::InvalidHandle)
		{
			do
			{
				std::string fileExt;

				std::string fileName = get_file_name(findData.name, &fileExt);

				std::string realDevPath = (pathPrefix + findData.name);

				// We process things depending on file extension, for now.
				if (stricmp(fileExt.c_str(), "TXD") == 0)
				{
					// Register this TXD.
					theGame->GetTextureManager().RegisterResource(fileName, imgDevice, realDevPath);
				}
			} while (imgDevice->FindNext(findHandle, &findData));

			imgDevice->FindClose(findHandle);
		}
	}

	// Alright, let's pray for the best!
}

inline bool is_white_space(char c)
{
	return (c == ' ' || c == '\t');
}

static inline std::list<std::string> split_cmd_line(std::string line)
{
	std::list<std::string> argsOut;

	std::string currentArg;

	for (char c : line)
	{
		if (!is_white_space(c))
		{
			currentArg += c;
		}
		else if (currentArg.empty() == false)
		{
			argsOut.push_back(std::move(currentArg));
		}
	}

	if (currentArg.empty() == false)
	{
		argsOut.push_back(std::move(currentArg));
	}

	return argsOut;
}

static inline std::vector<std::string> split_csv_args(std::string line)
{
	std::vector<std::string> argsOut;

	const char* linePtr = line.c_str();

	const char* curLineStart = NULL;
	const char* curLineEnd   = NULL;

	while (char c = *linePtr)
	{
		bool wantsTokenTermination = false;
		bool wantsTokenAdd         = false;

		if (c == ',')
		{
			size_t curLineLen = (curLineEnd - curLineStart);

			if (curLineLen != 0)
			{
				wantsTokenTermination = true;
				wantsTokenAdd         = true;
			}
		}
		else
		{
			if (curLineStart == NULL)
			{
				if (!is_white_space(c))
				{
					curLineStart = linePtr;
				}
			}
			else
			{
				if (is_white_space(c))
				{
					wantsTokenTermination = true;
				}
			}
		}

		if (wantsTokenTermination)
		{
			curLineEnd = linePtr;
		}

		if (wantsTokenAdd)
		{
			argsOut.push_back(std::string(curLineStart, curLineEnd));

			curLineStart = NULL;
			curLineEnd   = NULL;
		}

		linePtr++;
	}

	// Push ending token.
	{
		size_t curLineLen = (linePtr - curLineStart);

		if (curLineLen != 0)
		{
			argsOut.push_back(std::string(curLineStart, linePtr));
		}
	}

	return argsOut;
}

static inline std::string get_cfg_line(std::istream& in)
{
	std::string line;

	std::getline(in, line);

	const char* lineIter = line.c_str();

	const char* lineStartPos = lineIter;

	while (char c = *lineIter)
	{
		if (!is_white_space(c))
		{
			if (lineStartPos == NULL)
			{
				lineStartPos = lineIter;
			}
		}

		if (c == '\r')
		{
			break;
		}

		lineIter++;
	}

	return std::string(lineStartPos, lineIter);
}

inline bool ignore_line(const std::string& line)
{
	return (line.empty() || line.front() == '#');
}

// Yea rite, lets just do this.
inline std::stringstream get_file_stream(const std::string& absPath)
{
	vfs::StreamPtr filePtr = vfs::OpenRead(absPath);

	if (filePtr == NULL)
		return std::stringstream();

	std::vector<std::uint8_t> fileData = filePtr->ReadToEnd();

	return std::stringstream(std::string(fileData.begin(), fileData.end()), std::ios::in);
}

static thread_local GameUniversePtr g_currentParseUniverse;

using LineHandler     = void (*)(const std::string& line);
using FinalizeHandler = void (*)();

struct SectionDescriptor
{
	const char* typeName;
	LineHandler handler;
	FinalizeHandler finalizeHandler;

	SectionDescriptor(const char* typeName, LineHandler handler)
	    : typeName(typeName), handler(handler), finalizeHandler(nullptr)
	{
	}

	SectionDescriptor(const char* typeName, LineHandler handler, FinalizeHandler finalizeHandler)
	    : typeName(typeName), handler(handler), finalizeHandler(finalizeHandler)
	{
	}
};

static const SectionDescriptor* FindSectionDescriptor(const SectionDescriptor* descriptorList, const std::string& section)
{
	for (; descriptorList->typeName; descriptorList++)
	{
		if (descriptorList->typeName == section)
		{
			return descriptorList;
		}
	}

	return nullptr;
}

static void ProcessSectionedFile(const SectionDescriptor* descriptorList, std::stringstream& fileStream)
{
	// Process entries
	bool isInSection = false;

	const SectionDescriptor* curDescriptor = nullptr;

	while (!fileStream.eof())
	{
		std::string cfgLine;

		cfgLine = get_cfg_line(fileStream);

		// Not that hard to process, meow.
		if (ignore_line(cfgLine) == false)
		{
			if (!isInSection)
			{
				curDescriptor = FindSectionDescriptor(descriptorList, cfgLine);

				isInSection = true;
			}
			else
			{
				if (cfgLine == "end")
				{
					if (curDescriptor && curDescriptor->finalizeHandler)
					{
						curDescriptor->finalizeHandler();
					}

					isInSection = false;
				}
				else
				{
					if (curDescriptor)
					{
						curDescriptor->handler(cfgLine);
					}
				}
			}
		}
	}

	// Maybe they forgot an "end" statement, so make sure things are finalized properly.
	if (isInSection)
	{
		if (curDescriptor && curDescriptor->finalizeHandler)
		{
			curDescriptor->finalizeHandler();
		}
	}
}

static void HandleObjectLine(const std::string& line)
{
	std::vector<std::string> args = split_csv_args(line);

	// There are actually different model types defined by how many entries are in the line.
	// This is pretty clever.
	bool isNonBreakable     = false;
	bool isBreakable        = false;
	bool isComplexBreakable = false;

	streaming::ident_t id    = 0;
	streaming::ident_t newID = 0;

	// TODO: actually properly parse each model type and create appropriate model infos for them.

	if (args.size() == 5) // Non-breakable (SA)
	{
		isNonBreakable = true;

		// Process this. :)
		id                           = atoi(args[0].c_str());
		const std::string& modelName = args[1];
		const std::string& txdName   = args[2];
		double lodDistance           = atof(args[3].c_str());
		std::uint32_t flags          = atoi(args[4].c_str());

		newID = theGame->GetModelManager().RegisterAtomicModel(modelName, txdName, (float)lodDistance, flags, g_currentParseUniverse->GetImageMountPoint() + modelName + ".dff");
	}
	else if (args.size() == 6)
	{
		isNonBreakable = true;

		id                           = atoi(args[0].c_str());
		const std::string& modelName = args[1];
		const std::string& txdName   = args[2];
		int meshCount                = atoi(args[3].c_str()); // should be 1.
		float drawDistance           = (float)atof(args[4].c_str());
		std::uint32_t flags          = atoi(args[5].c_str());

		newID = theGame->GetModelManager().RegisterAtomicModel(modelName, txdName, drawDistance, flags, g_currentParseUniverse->GetImageMountPoint() + modelName + ".dff");
	}
	else if (args.size() == 7)
	{
		isBreakable = true;

		id                           = atoi(args[0].c_str());
		const std::string& modelName = args[1];
		const std::string& txdName   = args[2];
		int meshCount                = atoi(args[3].c_str()); // should be 1.
		float drawDistance           = (float)atof(args[4].c_str());
		float drawDistance2          = (float)atof(args[5].c_str());
		std::uint32_t flags          = atoi(args[6].c_str());

		newID = theGame->GetModelManager().RegisterAtomicModel(modelName, txdName, drawDistance, flags, g_currentParseUniverse->GetImageMountPoint() + modelName + ".dff");
	}
	else if (args.size() == 8)
	{
		isComplexBreakable = true;

		id                           = atoi(args[0].c_str());
		const std::string& modelName = args[1];
		const std::string& txdName   = args[2];
		int meshCount                = atoi(args[3].c_str()); // should be 1.
		float drawDistance           = (float)atof(args[4].c_str());
		float drawDistance2          = (float)atof(args[5].c_str());
		float drawDistance3          = (float)atof(args[6].c_str());
		std::uint32_t flags          = atoi(args[7].c_str());

		newID = theGame->GetModelManager().RegisterAtomicModel(modelName, txdName, drawDistance, flags, g_currentParseUniverse->GetImageMountPoint() + modelName + ".dff");
	}

	if (g_currentParseUniverse)
	{
		// register us as owning this streaming index
		g_currentParseUniverse->RegisterOwnedStreamingIndex(newID);

		// and register the model index mapping as well
		g_currentParseUniverse->RegisterModelIndexMapping(id, newID);
	}

	// TODO: is there any different format with less or more arguments?
	// maybe for IPL?
}

static void HandleTimedObjectLine(const std::string& line)
{
	std::vector<std::string> args = split_csv_args(line);

	streaming::ident_t id    = 0;
	streaming::ident_t newID = 0;

	if (args.size() == 8) // GTA3, at the least?
	{
		id                           = atoi(args[0].c_str());
		const std::string& modelName = args[1];
		const std::string& txdName   = args[2];
		int meshCount                = atoi(args[3].c_str()); // should be 1.
		float drawDistance           = (float)atof(args[4].c_str());
		std::uint32_t flags          = atoi(args[5].c_str());
		int onHour                   = atoi(args[6].c_str());
		int offHour                  = atoi(args[7].c_str());

		newID = theGame->GetModelManager().RegisterAtomicModel(modelName, txdName, drawDistance, flags, g_currentParseUniverse->GetImageMountPoint() + modelName + ".dff");
	}

	if (g_currentParseUniverse)
	{
		// register us as owning this streaming index
		g_currentParseUniverse->RegisterOwnedStreamingIndex(newID);

		// and register the model index mapping as well
		g_currentParseUniverse->RegisterModelIndexMapping(id, newID);
	}
}

static void HandleTxdParentLine(const std::string& line)
{
	std::vector<std::string> args = split_csv_args(line);

	if (args.size() == 2)
	{
		const std::string& txdName       = args[0].c_str();
		const std::string& txdParentName = args[1].c_str();

		// Register a TXD dependency.
		theGame->GetTextureManager().SetTexParent(txdName, txdParentName);
	}
}

static const SectionDescriptor ideSections[] = {
    {"objs", HandleObjectLine},
    {"tobj", HandleTimedObjectLine},
    {"txdp", HandleTxdParentLine},
    {nullptr, nullptr}};

void FileLoader::LoadIDEFile(const std::string& relPath, const GameUniversePtr& universe)
{
	std::stringstream ideFile = get_file_stream(relPath);

	// set the universe we parse into
	g_currentParseUniverse = universe;

	// debug print
	if (g_fileloaderDebug)
	{
		console::Printf("Loading IDE file %s into universe %s\n", relPath.c_str(), g_currentParseUniverse->GetConfiguration().gameName.c_str());
	}

	// load the section file
	ProcessSectionedFile(ideSections, ideFile);

	// unset the universe
	g_currentParseUniverse.reset();
}

struct sa_iplInstance_t
{
	rw::V3d position;              // 0
	rw::Quat quatRotation;         // 12
	streaming::ident_t modelIndex; // 28
	union {
		struct
		{
			unsigned char areaIndex;       // 32
			unsigned char unimportant : 1; // 33, not that important to keep streamed
			unsigned char unusedFlag1 : 1;
			unsigned char underwater : 1;    // entity is placed underwater
			unsigned char isTunnel : 1;      // is tunnel segment
			unsigned char isTunnelTrans : 1; // is tunnel transition segment
			unsigned char padFlags : 3;      // unused
			unsigned char pad[2];            // 34
		};
		unsigned int uiFlagNumber;
	};
	int lodIndex; // 36, index inside of the .ipl file pointing at the LOD instance.
};

// Code from MTA.
static void QuatToMatrix(const rw::Quat& q, rw::Matrix& m)
{
	float xx = q.x * q.x;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float xw = q.x * q.w;

	float yy = q.y * q.y;
	float yz = q.y * q.z;
	float yw = q.y * q.w;

	float zz = q.z * q.z;
	float zw = q.z * q.w;

	m.right.x = 1.0f - 2.0f * (yy + zz);
	m.right.y = 2.0f * (xy - zw);
	m.right.z = 2.0f * (xz + yw);

	m.up.x = 2.0f * (xy + zw);
	m.up.y = 1.0f - 2.0f * (xx + zz);
	m.up.z = 2.0f * (yz - xw);

	m.at.x = 2.0f * (xz - yw);
	m.at.y = 2.0f * (yz + xw);
	m.at.z = 1.0f - 2.0f * (xx + yy);
}

struct inst_section_manager
{
	inline void RegisterGTA3Instance(
	    streaming::ident_t modelID,
	    int areaCode, // optional: zero by default.
	    rw::V3d position,
	    rw::V3d scale,
	    rw::Quat rotation)
	{
		// I have no actual idea how things are made exactly, but lets just register it somehow.
		ModelManager::ModelResource* modelInfo = theGame->GetModelManager().GetModelByID(g_currentParseUniverse->GetModelIndexMapping(modelID));

		if (!modelInfo)
			return;

		Entity* resultEntity = new Entity(theGame);

		resultEntity->SetModelIndex(modelID);

		// Assign the matrix.
		{
			rw::Matrix instMatrix;

			QuatToMatrix(rotation, instMatrix);

			instMatrix.rightw = 0;
			instMatrix.atw    = 0;
			instMatrix.upw    = 0;
			instMatrix.pos    = position;
			instMatrix.posw   = 1;

			resultEntity->SetModelling(instMatrix);
		}

		resultEntity->interiorId = areaCode;

		// Register this instance entity.
		// Note that we have no support for LOD instances here.
		lod_inst_entity inst_info;
		inst_info.lod_id = -1;
		inst_info.entity = resultEntity;

		this->instances.push_back(std::move(inst_info));
	}

	inline void RegisterBinarySAInstance(sa_iplInstance_t& instData)
	{
		ModelManager::ModelResource* modelInfo = theGame->GetModelManager().GetModelByID(g_currentParseUniverse->GetModelIndexMapping(instData.modelIndex));

		if (!modelInfo)
			return;

		Entity* resultEntity = NULL;

		// TODO: check dynamic object properties.
		// if dynamic property exists, this is actually an object.
		{
			// TODO: there can be some weird buildings too.

			resultEntity = new Entity(theGame);

			resultEntity->SetModelIndex(instData.modelIndex);

			// dont write z buffer flag?
		}

		// Convert the Quat to a matrix and assign it.
		{
			rw::Matrix instMatrix;

			QuatToMatrix(instData.quatRotation, instMatrix);

			instMatrix.rightw = 0;
			instMatrix.atw    = 0;
			instMatrix.upw    = 0;
			instMatrix.pos    = instData.position;
			instMatrix.posw   = 1;

			resultEntity->SetModelling(instMatrix);
		}

		// Set flags.
		if (instData.underwater)
		{
			resultEntity->isUnderwater = true;
		}

		if (instData.isTunnel)
		{
			resultEntity->isTunnelObject = true;
		}

		if (instData.isTunnelTrans)
		{
			resultEntity->isTunnelTransition = true;
		}

		if (instData.unimportant)
		{
			resultEntity->isUnimportantToStreamer = true;
		}

		resultEntity->interiorId = instData.areaIndex;

		// Register this instance entity.
		lod_inst_entity inst_info;
		inst_info.lod_id = instData.lodIndex;
		inst_info.entity = resultEntity;

		this->instances.push_back(std::move(inst_info));
	}

	inline void Finalize(void)
	{
		// Map LOD instances.
		size_t numInstances = this->instances.size();

		for (const lod_inst_entity& inst : this->instances)
		{
			int lod_id = inst.lod_id;

			Entity* baseEntity = inst.entity;

			// Set up LOD entity link.
			if (lod_id > 0 && lod_id <= numInstances)
			{
				const lod_inst_entity& lod_inst = this->instances[lod_id - 1];

				Entity* lodEntity = lod_inst.entity;

				if (baseEntity != lodEntity && lodEntity->IsLowerLODOf(baseEntity) == false)
				{
					baseEntity->SetLODEntity(lodEntity);
				}
			}

			// Register this entity into the world.
			baseEntity->LinkToWorld(theGame->GetWorld());
		}

		// If certain entities have LOD "models" and their entities do not have lod LODs already.
		// Then automatically create lower LODs for them.
		for (const lod_inst_entity& inst : this->instances)
		{
			Entity* entity = inst.entity;

			if (entity->GetLODEntity() == NULL)
			{
				ModelManager::ModelResource* modelEntry = entity->GetModelInfo();

				if (modelEntry)
				{
					ModelManager::ModelResource* lodModel = modelEntry->GetLODModel();

					if (lodModel)
					{
						// Actually create an automatic LOD instance placed at the exact same position.
						Entity* lodInst = new Entity(entity->GetGame());

						lodInst->SetModelIndex(lodModel->GetID());
						lodInst->SetModelling(entity->GetModelling());

						entity->SetLODEntity(lodInst);

						lodInst->LinkToWorld(theGame->GetWorld());
					}
				}
			}
		}

		// Success.
		this->instances.clear();
	}

  private:
	struct lod_inst_entity
	{
		int lod_id;
		Entity* entity;
	};

	std::vector<lod_inst_entity> instances;
};

static thread_local inst_section_manager inst_sec_man;

static void HandleInstLine(const std::string& line)
{
	std::vector<std::string> args = split_csv_args(line);

	if (args.size() == 11)
	{
		// San Andreas map line.
		// We should kinda do what the GTA:SA engine does here.
		sa_iplInstance_t iplInst;
		iplInst.modelIndex     = atoi(args[0].c_str());
		iplInst.uiFlagNumber   = atoi(args[2].c_str());
		iplInst.position.x     = (float)atof(args[3].c_str());
		iplInst.position.y     = (float)atof(args[4].c_str());
		iplInst.position.z     = (float)atof(args[5].c_str());
		iplInst.quatRotation.x = (float)atof(args[6].c_str());
		iplInst.quatRotation.y = (float)atof(args[7].c_str());
		iplInst.quatRotation.z = (float)atof(args[8].c_str());
		iplInst.quatRotation.w = (float)atof(args[9].c_str());
		iplInst.lodIndex       = atoi(args[10].c_str());

		const std::string& modelName = args[1];

		inst_sec_man.RegisterBinarySAInstance(iplInst);
	}
	else if (args.size() == 12)
	{
		// GTA3/VC map line.
		streaming::ident_t modelID = atoi(args[0].c_str());
		rw::V3d pos(
		    (float)atof(args[2].c_str()),
		    (float)atof(args[3].c_str()),
		    (float)atof(args[4].c_str()));
		rw::V3d scale(
		    (float)atof(args[5].c_str()),
		    (float)atof(args[6].c_str()),
		    (float)atof(args[7].c_str()));
		rw::Quat rotQuat;
		rotQuat.x = (float)atof(args[8].c_str());
		rotQuat.y = (float)atof(args[9].c_str());
		rotQuat.z = (float)atof(args[10].c_str());
		rotQuat.w = (float)atof(args[11].c_str());

		inst_sec_man.RegisterGTA3Instance(
		    modelID, 0,
		    pos, scale, rotQuat);
	}
	else if (args.size() == 13)
	{
		// Vice City map line.
		streaming::ident_t modelID = atoi(args[0].c_str());
		int areaCode               = atoi(args[2].c_str());
		rw::V3d pos(
		    (float)atof(args[3].c_str()),
		    (float)atof(args[4].c_str()),
		    (float)atof(args[5].c_str()));
		rw::V3d scale(
		    (float)atof(args[6].c_str()),
		    (float)atof(args[7].c_str()),
		    (float)atof(args[8].c_str()));
		rw::Quat rotQuat;
		rotQuat.x = (float)atof(args[9].c_str());
		rotQuat.y = (float)atof(args[10].c_str());
		rotQuat.z = (float)atof(args[11].c_str());
		rotQuat.w = (float)atof(args[12].c_str());

		inst_sec_man.RegisterGTA3Instance(
		    modelID, areaCode,
		    pos, scale, rotQuat);
	}
}

static void HandleInstEnd()
{
	inst_sec_man.Finalize();
}

const static SectionDescriptor iplSections[] = {
    {"inst", HandleInstLine, HandleInstEnd},
    {nullptr, nullptr}};

// Data file loaders!
void FileLoader::LoadIPLFile(const std::string& absPath, const GameUniversePtr& universe)
{
	std::stringstream iplFileData = get_file_stream(absPath);

	// set the universe we parse into
	g_currentParseUniverse = universe;

	// debug print
	if (g_fileloaderDebug)
	{
		console::Printf("Loading IPL file %s into universe %s\n", absPath.c_str(), g_currentParseUniverse->GetConfiguration().gameName.c_str());
	}

	// load the section file
	ProcessSectionedFile(iplSections, iplFileData);

	// unset the universe
	g_currentParseUniverse.reset();
}

static ConVar<bool> g_fileloaderDebugVar("fileLoader_debug", ConVar_Archive, false, &g_fileloaderDebug);
}