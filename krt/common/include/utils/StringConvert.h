#pragma once

namespace krt
{
std::wstring ToWide(const std::string& narrow);
std::string ToNarrow(const std::wstring& wide);
}