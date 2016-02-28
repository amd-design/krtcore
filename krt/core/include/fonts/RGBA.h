#pragma once

namespace krt
{
struct RGBA
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;

	inline RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: red(r), green(g), blue(b), alpha(a)
	{

	}

	inline RGBA()
		: RGBA(0, 0, 0, 255)
	{

	}

	inline RGBA(uint8_t r, uint8_t g, uint8_t b)
		: RGBA(r, g, b, 255)
	{

	}

	inline static RGBA FromFloat(float r, float g, float b, float a)
	{
		return RGBA((uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f), (uint8_t)(a * 255.0f));
	}

	inline static RGBA FromARGB(uint32_t argb)
	{
		return RGBA((argb & 0xFF0000) >> 16, ((argb & 0xFF00) >> 8), argb & 0xFF, (argb & 0xFF000000) >> 24);
	}

	inline uint32_t AsARGB() const
	{
		return (alpha << 24) | (red << 16) | (green << 8) | blue;
	}
};
}