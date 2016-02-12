#include <StdInc.h>
#include <vfs/Manager.h>

#include <vfs/Win32Device.h>

namespace krt
{
namespace vfs
{
Manager::Manager()
{
	m_fallbackDevice = std::make_shared<Win32Device>();
}

StreamPtr Manager::OpenRead(const std::string& path)
{
	auto device = GetDevice(path);

	if (device)
	{
		auto handle = device->Open(path, true);

		if (handle != INVALID_DEVICE_HANDLE)
		{
			return std::make_shared<Stream>(device, handle);
		}
	}

	return nullptr;
}

DevicePtr Manager::GetDevice(const std::string& path)
{
	std::lock_guard<std::recursive_mutex> lock(m_mountMutex);

	// if only one device exists for a chosen prefix, we want to always return that device
	// if multiple exists, we only want to return a device if there's a file/directory entry by the path we specify

	for (const auto& mount : m_mounts)
	{
		// if the prefix patches
		if (strncmp(path.c_str(), mount.prefix.c_str(), mount.prefix.length()) == 0)
		{
			// single device case
			if (mount.devices.size() == 1)
			{
				return mount.devices[0];
			}
			else
			{
				// check each device assigned to the mount point
				Device::THandle handle;

				for (const auto& device : mount.devices)
				{
					if ((handle = device->Open(path, true)) != Device::InvalidHandle)
					{
						device->Close(handle);

						return device;
					}
				}

				// as this is a valid mount, but no valid file, bail out with nothing
				return nullptr;
			}
		}
	}

	return m_fallbackDevice;
}

void Manager::Mount(const DevicePtr& device, const std::string& path)
{
	std::lock_guard<std::recursive_mutex> lock(m_mountMutex);

	// find an existing mount in the mount list to add to
	for (auto&& mount : m_mounts)
	{
		if (mount.prefix == path)
		{
			mount.devices.push_back(device);
			return;
		}
	}

	// if we're here, we didn't find any existing device - add a new one instead
	{
		MountPoint mount;
		mount.prefix = path;
		mount.devices.push_back(device);

		m_mounts.insert(mount);
	}
}

void Manager::Unmount(const std::string& path)
{
	std::lock_guard<std::recursive_mutex> lock(m_mountMutex);

	// we need to manually iterate to be able to use erase(iterator)
	for (auto it = m_mounts.begin(); it != m_mounts.end(); )
	{
		const auto& mount = *it;

		if (mount.prefix == path)
		{
			it = m_mounts.erase(it);
		}
		else
		{
			++it;
		}
	}
}

static Manager TheManager;

StreamPtr OpenRead(const std::string& path)
{
	return TheManager.OpenRead(path);
}

DevicePtr GetDevice(const std::string& path)
{
	return TheManager.GetDevice(path);
}

void Mount(const DevicePtr& device, const std::string& path)
{
	TheManager.Mount(device, path);
}

void Unmount(const std::string& path)
{
	TheManager.Unmount(path);
}
}
}