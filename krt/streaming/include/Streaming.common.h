#pragma once

// Common utilities like resource locations that are used quite often

#include "Streaming.h"
#include "vfs\Device.h"

namespace krt
{

// Default data provider from the VFS.
class DeviceResourceLocation : public streaming::ResourceLocation
{
  private:
	std::string m_path;
	vfs::DevicePtr m_device;

	size_t m_length;

  public:
	DeviceResourceLocation(vfs::DevicePtr device, std::string path)
	{
		m_device = device;
		m_length = device->GetLength(path);
		m_path   = std::move(path);

		assert(m_length != -1);
	}

	size_t getDataSize(void) const override
	{
		return m_length;
	}

	void fetchData(void* dataBuf) override
	{
		uint64_t ptr;
		auto handle = m_device->OpenBulk(m_path, &ptr);

        if ( handle == vfs::Device::InvalidHandle )
        {
            throw std::exception( "failed to open bulk handle" );
        }

		m_device->ReadBulk(handle, ptr, dataBuf, m_length);
		m_device->CloseBulk(handle);
	}
};

}