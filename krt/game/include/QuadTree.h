#pragma once

// Basic QuadTree implementation for world sectoring.
// It works recursively until a predefined depth.

#include "WorldMath.h"

#include <type_traits>

namespace krt
{

template <size_t depth, size_t boundDimm, typename DataType>
struct QuadTree
{
	template <typename numberType, size_t minX, size_t minY, size_t maxX, size_t maxY>
	MATH_INLINE static bool IsPointInBounds(numberType x, numberType y)
	{
		return (x >= minX && x >= minY && x < maxX && y < maxY);
	}

	template <typename numberType, size_t minX, size_t minY, size_t maxX, size_t maxY>
	MATH_INLINE static bool IsBoundsInBounds(numberType box_minX, numberType box_minY, numberType box_maxX, numberType box_maxY)
	{
		typedef sliceOfData<numberType> qt_slice_t;

		const qt_slice_t width_slice_sector(minX, maxX - minX);
		const qt_slice_t height_slice_sector(minY, maxY - minY);

		const qt_slice_t width_slice_box(box_minX, box_maxX - box_minX);
		const qt_slice_t height_slice_box(box_minY, box_maxY - box_minY);

		qt_slice_t::eIntersectionResult width_intersect_result  = width_slice_sector.intersectWith(width_slice_box);
		qt_slice_t::eIntersectionResult height_intersect_result = height_slice_sector.intersectWith(height_slice_box);

		bool isIntersecting =
		    qt_slice_t::isFloatingIntersect(width_intersect_result) == false &&
		    qt_slice_t::isFloatingIntersect(height_intersect_result) == false;

		return isIntersecting;
	}

	template <size_t depth, size_t boundDimm, typename DataType, size_t xOff, size_t yOff>
	struct Node
	{
		static size_t constexpr boundDimm = boundDimm;

	private:
		static constexpr size_t subdimm_bound = (boundDimm / 2);

		Node<depth - 1, subdimm_bound, DataType, xOff, yOff> topLeft;
		Node<depth - 1, subdimm_bound, DataType, xOff + subdimm_bound, yOff> topRight;
		Node<depth - 1, subdimm_bound, DataType, xOff, yOff + subdimm_bound> bottomLeft;
		Node<depth - 1, subdimm_bound, DataType, xOff + subdimm_bound, yOff + subdimm_bound> bottomRight;

	public:
		template <typename numberType, typename callbackType>
		MATH_INLINE void VisitByPoint(numberType x, numberType y, callbackType& cb)
		{
			// Each node takes care of bound checking itself.
			if (IsPointInBounds<numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm>(x, y))
			{
				topLeft.VisitByPoint(x, y, cb);
				topRight.VisitByPoint(x, y, cb);
				bottomLeft.VisitByPoint(x, y, cb);
				bottomRight.VisitByPoint(x, y, cb);
			}
		}

		template <typename numberType, typename callbackType>
		MATH_INLINE void VisitByBounds(numberType minX, numberType minY, numberType maxX, numberType maxY, callbackType& cb)
		{
			// Taking care of our bounds ourselves.
			if (IsBoundsInBounds<numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm>(minX, minY, maxX, maxY))
			{
				topLeft.VisitByBounds(minX, minY, maxX, maxY, cb);
				topRight.VisitByBounds(minX, minY, maxX, maxY, cb);
				bottomLeft.VisitByBounds(minX, minY, maxX, maxY, cb);
				bottomRight.VisitByBounds(minX, minY, maxX, maxY, cb);
			}
		}

		template <typename callbackType>
		MATH_INLINE void ForAllEntries(callbackType& cb)
		{
			topLeft.ForAllEntries(cb);
			topRight.ForAllEntries(cb);
			bottomLeft.ForAllEntries(cb);
			bottomRight.ForAllEntries(cb);
		}
	};

	template <size_t boundDimm, typename DataType, size_t xOff, size_t yOff>
	struct Node<0, boundDimm, DataType, xOff, yOff>
	{
		static size_t constexpr boundDimm = boundDimm;

		inline Node(void) : data(xOff, yOff, xOff + boundDimm, yOff + boundDimm)
		{
			// We want each data to know about its bounds.
			return;
		}

	private:
		DataType data;

	public:
		template <typename numberType, typename callbackType>
		MATH_INLINE void VisitByPoint(numberType x, numberType y, callbackType& cb)
		{
			// Each node takes care of bound checking itself.
			if (IsPointInBounds<numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm>(x, y))
			{
				// Call the thing.
				cb(this->data);
			}
		}

		template <typename numberType, typename callbackType>
		MATH_INLINE void VisitByBounds(numberType minX, numberType minY, numberType maxX, numberType maxY, callbackType& cb)
		{
			if (IsBoundsInBounds<numberType, xOff, yOff, xOff + boundDimm, yOff + boundDimm>(minX, minY, maxX, maxY))
			{
				cb(this->data);
			}
		}

		template <typename callbackType>
		MATH_INLINE void ForAllEntries(callbackType& cb)
		{
			cb(this->data);
		}
	};

	Node<depth, boundDimm, DataType, 0, 0> root;
};
}