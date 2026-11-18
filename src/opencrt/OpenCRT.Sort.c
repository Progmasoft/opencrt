/*
 * SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/** The comparison of qsort and bsearch. */
typedef int (*tFn_Compare)(const void*, const void*);

/** Below this many elements, insertion sort is faster than splitting. */
#define OPENCRT_SORT_SMALL 12

/**
 * @brief Swaps two elements byte by byte.
 *
 * @param[in,out] aInOut_pFirst  The first element.
 * @param[in,out] aInOut_pSecond The second element.
 * @param[in]     aSize          The size of an element.
 */
static void
Swap(unsigned char* aInOut_pFirst, unsigned char* aInOut_pSecond, size_t aSize)
{
        if(aInOut_pFirst == aInOut_pSecond)
                return;
        for(size_t index = 0; index < aSize; index++)
        {
                unsigned char byte      = aInOut_pFirst[ index ];
                aInOut_pFirst[ index ]  = aInOut_pSecond[ index ];
                aInOut_pSecond[ index ] = byte;
        }
}

/**
 * @brief Sorts a few elements by insertion.
 *
 * @param[in,out] aInOut_pBase The first element.
 * @param[in]     aCount       How many.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 */
static void
InsertionSort(unsigned char* aInOut_pBase, size_t aCount, size_t aSize, tFn_Compare aFn_Compare)
{
        for(size_t i = 1; i < aCount; i++)
        {
                for(size_t j = i; j > 0
                    && aFn_Compare(aInOut_pBase + (j - 1) * aSize, aInOut_pBase + j * aSize) > 0;
                    j--)
                        Swap(aInOut_pBase + (j - 1) * aSize, aInOut_pBase + j * aSize, aSize);
        }
}

/**
 * @brief Moves an element down a heap until both its children are not greater.
 *
 * @param[in,out] aInOut_pBase The heap.
 * @param[in]     aRoot        The element.
 * @param[in]     aCount       The size of the heap.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 */
static void
SiftDown(unsigned char* aInOut_pBase,
         size_t         aRoot,
         size_t         aCount,
         size_t         aSize,
         tFn_Compare    aFn_Compare)
{
        for(size_t child; (child = 2 * aRoot + 1) < aCount; aRoot = child)
        {
                if(child + 1 < aCount
                   && aFn_Compare(aInOut_pBase + child * aSize, aInOut_pBase + (child + 1) * aSize)
                           < 0)
                        child++;
                if(aFn_Compare(aInOut_pBase + aRoot * aSize, aInOut_pBase + child * aSize) >= 0)
                        return;
                Swap(aInOut_pBase + aRoot * aSize, aInOut_pBase + child * aSize, aSize);
        }
}

/**
 * @brief Sorts by heap sort, in O(n log n) whatever the order of the input.
 *
 * @param[in,out] aInOut_pBase The first element.
 * @param[in]     aCount       How many.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 */
static void
HeapSort(unsigned char* aInOut_pBase, size_t aCount, size_t aSize, tFn_Compare aFn_Compare)
{
        for(size_t root = aCount / 2; root-- > 0;)
                SiftDown(aInOut_pBase, root, aCount, aSize, aFn_Compare);
        for(size_t end = aCount; end-- > 1;)
        {
                Swap(aInOut_pBase, aInOut_pBase + end * aSize, aSize);
                SiftDown(aInOut_pBase, 0, end, aSize, aFn_Compare);
        }
}

/**
 * @brief Puts the median of the first, middle and last elements first, as the pivot.
 *
 * @param[in,out] aInOut_pBase The first element.
 * @param[in]     aCount       How many, at least 3.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 */
static void
ChoosePivot(unsigned char* aInOut_pBase, size_t aCount, size_t aSize, tFn_Compare aFn_Compare)
{
        unsigned char* pMiddle = aInOut_pBase + aCount / 2 * aSize;
        unsigned char* pLast   = aInOut_pBase + (aCount - 1) * aSize;

        if(aFn_Compare(pMiddle, aInOut_pBase) < 0)
                Swap(pMiddle, aInOut_pBase, aSize);
        if(aFn_Compare(pLast, pMiddle) < 0)
        {
                Swap(pLast, pMiddle, aSize);
                if(aFn_Compare(pMiddle, aInOut_pBase) < 0)
                        Swap(pMiddle, aInOut_pBase, aSize);
        }
        Swap(aInOut_pBase, pMiddle, aSize);
}

/**
 * @brief Splits around the first element: those before the returned index are not greater, those
 *        after are not less.
 *
 * @param[in,out] aInOut_pBase The first element, the pivot.
 * @param[in]     aCount       How many.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 * @return Where the pivot ends.
 */
static size_t
Partition(unsigned char* aInOut_pBase, size_t aCount, size_t aSize, tFn_Compare aFn_Compare)
{
        size_t low  = 1;
        size_t high = aCount - 1;

        for(;;)
        {
                while(low <= high && aFn_Compare(aInOut_pBase + low * aSize, aInOut_pBase) < 0)
                        low++;
                while(high >= low && aFn_Compare(aInOut_pBase + high * aSize, aInOut_pBase) > 0)
                        high--;
                if(low >= high)
                        break;
                Swap(aInOut_pBase + low * aSize, aInOut_pBase + high * aSize, aSize);
                low++;
                high--;
        }
        Swap(aInOut_pBase, aInOut_pBase + high * aSize, aSize);
        return (high);
}

/**
 * @brief Introsort: quicksort while its depth stays under 2 log2 n, heap sort past it, so no input
 *        takes more than O(n log n) comparisons.
 *
 * @param[in,out] aInOut_pBase The first element.
 * @param[in]     aCount       How many.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison.
 * @param[in]     aDepth       How many more splits are allowed.
 */
static void
IntroSort(unsigned char* aInOut_pBase,
          size_t         aCount,
          size_t         aSize,
          tFn_Compare    aFn_Compare,
          int            aDepth)
{
        while(aCount > OPENCRT_SORT_SMALL)
        {
                if(aDepth-- == 0)
                {
                        HeapSort(aInOut_pBase, aCount, aSize, aFn_Compare);
                        return;
                }
                ChoosePivot(aInOut_pBase, aCount, aSize, aFn_Compare);
                size_t pivot = Partition(aInOut_pBase, aCount, aSize, aFn_Compare);

                /* Recurse into the smaller side, loop on the larger: the stack stays O(log n). */
                if(pivot < aCount - pivot - 1)
                {
                        IntroSort(aInOut_pBase, pivot, aSize, aFn_Compare, aDepth);
                        aInOut_pBase += (pivot + 1) * aSize;
                        aCount       -= pivot + 1;
                } else
                {
                        IntroSort(aInOut_pBase + (pivot + 1) * aSize,
                                  aCount - pivot - 1,
                                  aSize,
                                  aFn_Compare,
                                  aDepth);
                        aCount = pivot;
                }
        }
        InsertionSort(aInOut_pBase, aCount, aSize, aFn_Compare);
}

/**
 * @brief Sorts an array.
 *
 * @param[in,out] aInOut_pBase The array.
 * @param[in]     aCount       How many elements.
 * @param[in]     aSize        The size of an element.
 * @param[in]     aFn_Compare  The comparison, negative, zero or positive.
 */
void
qsort(void* aInOut_pBase, size_t aCount, size_t aSize, int (*aFn_Compare)(const void*, const void*))
{
        int depth = 0;

        if(aCount < 2 || aSize == 0)
                return;
        for(size_t count = aCount; count > 1; count >>= 1)
                depth += 2;
        IntroSort(aInOut_pBase, aCount, aSize, aFn_Compare, depth);
}

/**
 * @brief Finds an element in a sorted array.
 *
 * @param[in] aIn_pKey     The element to find.
 * @param[in] aIn_pBase    The array, sorted by aFn_Compare.
 * @param[in] aCount       How many elements.
 * @param[in] aSize        The size of an element.
 * @param[in] aFn_Compare  The comparison of the key with an element.
 * @return An element equal to the key, or NULL.
 */
void*
bsearch(const void* aIn_pKey,
        const void* aIn_pBase,
        size_t      aCount,
        size_t      aSize,
        int (*aFn_Compare)(const void*, const void*))
{
        const unsigned char* pBase = aIn_pBase;

        while(aCount > 0)
        {
                const unsigned char* pMiddle = pBase + aCount / 2 * aSize;
                int                  order   = aFn_Compare(aIn_pKey, pMiddle);

                if(order == 0)
                        return ((void*) pMiddle);
                if(order > 0)
                {
                        pBase   = pMiddle + aSize;
                        aCount -= aCount / 2 + 1;
                } else
                        aCount /= 2;
        }
        return (NULL);
}
