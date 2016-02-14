#pragma once

#include <vfs/Device.h>

#include <shared_mutex>

namespace krt
{
namespace streaming
{
class CdImageDevice : public vfs::Device
{
  public:
	CdImageDevice();

	virtual ~CdImageDevice() override;

  public:
	bool OpenImage(const std::string& imagePath);

	virtual THandle Open(const std::string& fileName, bool readOnly) override;

	virtual THandle OpenBulk(const std::string& fileName, uint64_t* ptr) override;

	virtual size_t Read(THandle handle, void* outBuffer, size_t size) override;

	virtual size_t ReadBulk(THandle handle, uint64_t ptr, void* outBuffer, size_t size) override;

	virtual size_t Seek(THandle handle, intptr_t offset, int seekType) override;

	virtual bool Close(THandle handle) override;

	virtual bool CloseBulk(THandle handle) override;

	virtual THandle FindFirst(const std::string& folder, vfs::FindData* findData) override;

	virtual bool FindNext(THandle handle, vfs::FindData* findData) override;

	virtual void FindClose(THandle handle) override;

	virtual void SetPathPrefix(const std::string& pathPrefix) override;

	virtual size_t GetLength(THandle handle) override;

	virtual size_t GetLength(const std::string& fileName) override;

  private:
	struct Entry
	{
		uint32_t offset;
		uint32_t size;
		char name[24];
	};

	struct HandleData
	{
		bool valid;
		Entry entry;
		size_t curOffset;

		inline HandleData()
		    : valid(false)
		{
		}
	};

	struct IgnoreCaseLess
	{
		bool operator()(const std::string& left, const std::string& right) const
		{
			return (_stricmp(left.c_str(), right.c_str()) < 0);
		}
	};

  private:
	const Entry* FindEntry(const std::string& path) const;

	HandleData* AllocateHandle(THandle* outHandle);

	HandleData* GetHandle(THandle inHandle);

	void FillFindData(vfs::FindData* findData, const Entry* entry);

  private:
	vfs::DevicePtr m_parentDevice;

	THandle m_parentHandle;

	uint64_t m_parentPtr;

	std::string m_pathPrefix;

	HandleData m_handles[16];

	std::vector<Entry> m_entries;

	std::map<std::string, Entry*, IgnoreCaseLess> m_entryLookup;

	// I do not know how what your vfs design will really be in the end,
	// so I use a simple shared access lock by following immutability rules.
	std::shared_timed_mutex lockDeviceConsistency;
};
}
}