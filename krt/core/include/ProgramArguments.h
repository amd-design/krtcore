#pragma once

namespace krt
{
class ProgramArguments
{
private:
	std::vector<std::string> m_arguments;

public:
	ProgramArguments(int argc, char** argv);

	inline const std::string& Get(int i) const
	{
		assert(i >= 0 && i < m_arguments.size());

		return m_arguments[i];
	}

	inline const std::string& operator[](int i) const
	{
		return Get(i);
	}

	inline size_t Count() const
	{
		return m_arguments.size();
	}
};
}