#pragma once

namespace krt
{
struct IgnoreCaseLess
{
	bool operator()(const std::string& left, const std::string& right) const
	{
		return (_stricmp(left.c_str(), right.c_str()) < 0);
	}
};
}