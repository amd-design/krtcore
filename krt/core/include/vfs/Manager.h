#pragma once

#include <vfs/Device.h>
#include <vfs/Stream.h>

namespace krt
{
namespace vfs
{
class Manager
{
private:
	struct MountPoint
	{
		std::string prefix;

		mutable std::vector<DevicePtr> devices; // mutable? MUTABLE? is this Rust?!
	};

	// sorts mount points based on longest prefix
	struct MountPointComparator
	{
		inline bool operator()(const MountPoint& left, const MountPoint& right) const
		{
			size_t lengthLeft = left.prefix.length();
			size_t lengthRight = right.prefix.length();

			if (lengthLeft == lengthRight)
			{
				return (left.prefix < right.prefix);
			}

			return (lengthLeft > lengthRight);
		}
	};

private:
	std::set<MountPoint, MountPointComparator> m_mounts;

	std::recursive_mutex m_mountMutex;

	// fallback device - usually a local file system implementation
	DevicePtr m_fallbackDevice;

public:
	Manager();

public:
	virtual StreamPtr OpenRead(const std::string& path);

	virtual DevicePtr GetDevice(const std::string& path);

	virtual void Mount(const DevicePtr& device, const std::string& path);

	virtual void Unmount(const std::string& path);
};

StreamPtr OpenRead(const std::string& path);

DevicePtr GetDevice(const std::string& path);

void Mount(const DevicePtr& device, const std::string& path);

void Unmount(const std::string& path);
}
}