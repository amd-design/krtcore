#include <CdImageDevice.h>
#include <StdInc.h>

#include <vfs/Manager.h>

#define CDIMAGE_SECTOR_SIZE 2048

namespace krt
{
namespace streaming
{
using vfs::Device;
using vfs::DevicePtr;

CdImageDevice::CdImageDevice()
    : m_parentHandle(InvalidHandle)
{
}

CdImageDevice::~CdImageDevice()
{
	// close the parent device if needed
	if (m_parentHandle != InvalidHandle)
	{
		m_parentDevice->CloseBulk(m_parentHandle);

		m_parentHandle = InvalidHandle;
	}
}

bool CdImageDevice::OpenImage(const std::string& imagePath)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxOpenOp(this->lockDeviceConsistency);

	// get the containing device, and early out if we don't have one
	DevicePtr parentDevice = vfs::GetDevice(imagePath);

	if (!parentDevice)
	{
		return false;
	}

	// attempt opening the archive from the specified path
	m_parentHandle = parentDevice->OpenBulk(imagePath, &m_parentPtr);

	// early out if invalid, again
	if (m_parentHandle == InvalidHandle)
	{
		return false;
	}

	m_parentDevice = parentDevice;

	// read the VER2 header, if any
	struct
	{
		char magic[4];
		uint32_t numEntries;
	} ver2header;

	if (m_parentDevice->ReadBulk(m_parentHandle, m_parentPtr, &ver2header, sizeof(ver2header)) != sizeof(ver2header))
	{
		return false;
	}

	// set the directory device handle/ptr if VER2, or load a directory file if not
	THandle directoryHandle;
	uint64_t directoryPtr;

	uint32_t numEntries;

	if (ver2header.magic[0] == 'V' && ver2header.magic[1] == 'E' && ver2header.magic[2] == 'R' && ver2header.magic[3] == '2')
	{
		// ver2 case (reopen to get a duplicate handle if needed)
		directoryHandle = parentDevice->OpenBulk(imagePath, &directoryPtr);

		directoryPtr += sizeof(ver2header);

		numEntries = ver2header.numEntries;
	}
	else
	{
		// ver1 case - attempt to open a .dir named similarly to the .img
		std::string directoryPath = imagePath.substr(0, imagePath.find_last_of('.')) + ".dir";

		if ((directoryHandle = parentDevice->OpenBulk(directoryPath, &directoryPtr)) == InvalidHandle)
		{
			// if we can't open the directory, bail out
			return false;
		}

		// use the directory path in case bulk handles are equivalent to the parent handle
		numEntries = parentDevice->GetLength(directoryPath) / sizeof(Entry);
	}

	// read the directory entry list
	size_t entriesSize = numEntries * sizeof(Entry);

	m_entries.resize(numEntries);

	if (parentDevice->ReadBulk(directoryHandle, directoryPtr, &m_entries[0], entriesSize) != entriesSize)
	{
		return false;
	}

	// calculate the entry hash map
	for (auto& entry : m_entries)
	{
		m_entryLookup[entry.name] = &entry;
	}

	// close the directory and return a success value
	parentDevice->CloseBulk(directoryHandle);

	return true;
}

// only THREAD-SAFE if called from SHARED-LOCK.
const CdImageDevice::Entry* CdImageDevice::FindEntry(const std::string& path) const
{
	// remove the path prefix
	std::string relativePath = path.substr(m_pathPrefix.length());

	// then, look up the entry in the lookup table
	const entryLookupMap_t::const_iterator it = m_entryLookup.find(relativePath);

	// if not found, return such
	if (it == m_entryLookup.end())
	{
		return nullptr;
	}

	// otherwise, return the entry
	return it->second;
}

// only THREAD-SAFE if called from EXCLUSIVE-LOCK.
CdImageDevice::HandleData* CdImageDevice::AllocateHandle(THandle* outHandle)
{
	for (int i = 0; i < _countof(m_handles); i++)
	{
		if (!m_handles[i].valid)
		{
			*outHandle = i;

			return &m_handles[i];
		}
	}

	return nullptr;
}

CdImageDevice::HandleData* CdImageDevice::GetHandle(THandle inHandle)
{
	if (inHandle >= 0 && inHandle < _countof(m_handles))
	{
		if (m_handles[inHandle].valid)
		{
			return &m_handles[inHandle];
		}
	}

	return nullptr;
}

CdImageDevice::THandle CdImageDevice::Open(const std::string& fileName, bool readOnly)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxOpenImageHandle(this->lockDeviceConsistency);

	// we only support read-only files
	if (readOnly)
	{
		auto entry = FindEntry(fileName);

		// if we found an entry
		if (entry)
		{
			// allocate a sequential handle
			THandle handle;
			auto handleData = AllocateHandle(&handle);

			if (handleData)
			{
				handleData->valid     = true;
				handleData->entry     = *entry;
				handleData->curOffset = 0;

				return handle;
			}
		}
	}

	return InvalidHandle;
}

CdImageDevice::THandle CdImageDevice::OpenBulk(const std::string& fileName, uint64_t* ptr)
{
	shared_lock_acquire<std::shared_timed_mutex> ctxBulkOperations(this->lockDeviceConsistency);

	auto entry = FindEntry(fileName);

	if (entry)
	{
		*ptr = entry->offset * CDIMAGE_SECTOR_SIZE;

		return reinterpret_cast<THandle>(entry);
	}

	return InvalidHandle;
}

size_t CdImageDevice::Read(THandle handle, void* outBuffer, size_t size)
{
	// EXLUSIVE LOCK IS REQUIRED, because you use a cached-handle approach.
	exclusive_lock_acquire<std::shared_timed_mutex> ctxBulkOperations(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		// detect EOF
		if (handleData->curOffset >= (handleData->entry.size * CDIMAGE_SECTOR_SIZE))
		{
			return 0;
		}

		// calculate and read
		size_t toRead = (handleData->entry.size * CDIMAGE_SECTOR_SIZE) - handleData->curOffset;

		if (toRead > size)
		{
			toRead = size;
		}

		size_t didRead = m_parentDevice->ReadBulk(m_parentHandle,
		                                          m_parentPtr + (handleData->entry.offset * CDIMAGE_SECTOR_SIZE) + handleData->curOffset,
		                                          outBuffer,
		                                          toRead);

		handleData->curOffset += didRead;

		return didRead;
	}

	return -1;
}

size_t CdImageDevice::ReadBulk(THandle handle, uint64_t ptr, void* outBuffer, size_t size)
{
	shared_lock_acquire<std::shared_timed_mutex> ctxBulkOperations(this->lockDeviceConsistency);

	return m_parentDevice->ReadBulk(m_parentHandle, m_parentPtr + ptr, outBuffer, size);
}

bool CdImageDevice::Close(THandle handle)
{
	// Again, shared handle approach.
	// We cannot let this go haywire.
	exclusive_lock_acquire<std::shared_timed_mutex> ctxBulkOperations(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		handleData->valid = false;

		return true;
	}

	return false;
}

bool CdImageDevice::CloseBulk(THandle handle)
{
	return true;
}

size_t CdImageDevice::Seek(THandle handle, intptr_t offset, int seekType)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxSeekHandle(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		size_t length = handleData->entry.size * CDIMAGE_SECTOR_SIZE;

		if (seekType == SEEK_CUR)
		{
			handleData->curOffset += offset;

			if (handleData->curOffset > length)
			{
				handleData->curOffset = length;
			}
		}
		else if (seekType == SEEK_SET)
		{
			handleData->curOffset = offset;
		}
		else if (seekType == SEEK_END)
		{
			handleData->curOffset = length - offset;
		}
		else
		{
			return -1;
		}

		return handleData->curOffset;
	}

	return -1;
}

size_t CdImageDevice::GetLength(THandle handle)
{
	shared_lock_acquire<std::shared_timed_mutex> ctxImmutableOp(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		return handleData->entry.size * CDIMAGE_SECTOR_SIZE;
	}

	return -1;
}

size_t CdImageDevice::GetLength(const std::string& fileName)
{
	shared_lock_acquire<std::shared_timed_mutex> ctxImmutableOp(this->lockDeviceConsistency);

	auto entry = FindEntry(fileName);

	if (entry)
	{
		return entry->size * CDIMAGE_SECTOR_SIZE;
	}

	return -1;
}

CdImageDevice::THandle CdImageDevice::FindFirst(const std::string& folder, vfs::FindData* findData)
{
	// You are allocating some kind of handles, which is not thread-safe.
	// So we gotta secure the device.
	exclusive_lock_acquire<std::shared_timed_mutex> ctxScanningOperation(this->lockDeviceConsistency);

	if (folder == m_pathPrefix)
	{
		THandle handle;
		auto handleData = AllocateHandle(&handle);

		if (handleData)
		{
			handleData->curOffset = 0;
			handleData->valid     = true;

			FillFindData(findData, &m_entries[handleData->curOffset]);

			return handle;
		}
	}

	return InvalidHandle;
}

bool CdImageDevice::FindNext(THandle handle, vfs::FindData* findData)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxScanningOperation(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		handleData->curOffset++;

		// have we passed the length already?
		if (handleData->curOffset >= m_entries.size())
		{
			return false;
		}

		// get the entry at the offset
		FillFindData(findData, &m_entries[handleData->curOffset]);

		// will the next increment go past the length? if not, return true
		return ((handleData->curOffset + 1) < m_entries.size());
	}

	return false;
}

void CdImageDevice::FindClose(THandle handle)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxScanningOperation(this->lockDeviceConsistency);

	auto handleData = GetHandle(handle);

	if (handleData)
	{
		handleData->valid = false;
	}
}

// only THREAD-SAFE if called from at least SHARED-LOCK.
void CdImageDevice::FillFindData(vfs::FindData* data, const Entry* entry)
{
	data->attributes = 0;
	data->length     = entry->size * CDIMAGE_SECTOR_SIZE;
	data->name       = entry->name;
}

void CdImageDevice::SetPathPrefix(const std::string& pathPrefix)
{
	exclusive_lock_acquire<std::shared_timed_mutex> ctxConfigChange(this->lockDeviceConsistency);

	m_pathPrefix = pathPrefix;
}
}
}