#include <StdInc.h>
#include <codecvt>

namespace krt
{
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> m_converter;

std::string ToNarrow(const std::wstring& wide)
{
	return m_converter.to_bytes(wide);
}

std::wstring ToWide(const std::string& narrow)
{
	return m_converter.from_bytes(narrow);
}
}