#pragma once

#include <vfs/Device.h>

namespace krt
{
namespace vfs
{
class Stream
{
private:
	DevicePtr m_device;

	Device::THandle m_handle;

public:
	Stream(const DevicePtr& device, Device::THandle handle);

	virtual ~Stream();

	std::vector<uint8_t> Read(size_t length);

	size_t Read(void* buffer, size_t length);

	inline size_t Read(std::vector<uint8_t>& buffer)
	{
		return Read(&buffer[0], buffer.size());
	}

	void Close();

	uint64_t GetLength();

	size_t Seek(intptr_t offset, int seekType);

	std::vector<uint8_t> ReadToEnd();
};

using StreamPtr = std::shared_ptr<Stream>;
}
}