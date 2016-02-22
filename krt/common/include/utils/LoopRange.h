#pragma once

namespace krt
{
template<typename T>
class LoopRange
{
public:
	class Iterator
	{
	private:
		T m_value;

	public:
		inline Iterator(T value)
			: m_value(value)
		{

		}

		bool operator==(const Iterator& right) const
		{
			return (m_value == right.m_value);
		}

		bool operator!=(const Iterator& right) const
		{
			return (m_value != right.m_value);
		}

		const T& operator*() const
		{
			return m_value;
		}

		Iterator& operator++()
		{
			++m_value;

			return *this;
		}
	};

private:
	const T m_from;
	const T m_to;

public:
	inline LoopRange(T to)
		: m_from(0), m_to(to)
	{

	}

	inline LoopRange(T from, T to)
		: m_from(from), m_to(to)
	{

	}

	inline Iterator begin() const
	{
		return{ m_from };
	}

	inline Iterator end() const
	{
		return{ m_to };
	}
};

template<typename T>
LoopRange<T> irange(T from, T to)
{
	static_assert(std::is_integral<T>::value, "irange() should be used with an integral type");

	return{ from, to };
}

template<typename T>
LoopRange<T> irange(T to)
{
	static_assert(std::is_integral<T>::value, "irange() should be used with an integral type");

	return{ to };
}
}