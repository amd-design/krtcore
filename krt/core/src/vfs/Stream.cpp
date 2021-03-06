#include <StdInc.h>
#include <vfs/Stream.h>

namespace krt
{
namespace vfs
{
Stream::Stream(const DevicePtr& device, Device::THandle handle)
	: m_device(device), m_handle(handle)
{

}

Stream::~Stream()
{
	Close();
}

size_t Stream::Read(void* buffer, size_t length)
{
	return m_device->Read(m_handle, buffer, length);
}

std::vector<uint8_t> Stream::Read(size_t length)
{
	std::vector<uint8_t> retval(length);
	length = Read(retval);

	retval.resize(length);

	return retval;
}

uint64_t Stream::GetLength()
{
	return m_device->GetLength(m_handle);
}

size_t Stream::Seek(intptr_t offset, int seekType)
{
	return m_device->Seek(m_handle, offset, seekType);
}

void Stream::Close()
{
	if (m_handle != INVALID_DEVICE_HANDLE)
	{
		m_device->Close(m_handle);
		m_handle = INVALID_DEVICE_HANDLE;
	}
}

std::vector<uint8_t> Stream::ReadToEnd()
{
	size_t fileLength = m_device->GetLength(m_handle);
	size_t curSize = Seek(0, SEEK_CUR);

	return Read(fileLength - curSize);
}
}
}