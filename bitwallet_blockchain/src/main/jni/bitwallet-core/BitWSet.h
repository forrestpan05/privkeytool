//
//  BitWSet.h
//
//  Created by Aaron Voisine on 9/11/15.
//  Copyright (c) 2015 bitwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BitWSet_h
#define BitWSet_h

#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BitWSetStruct BitWSet;

// retruns a newly allocated empty set that must be freed by calling BitWSetFree()
// size_t hash(const void *) is a function that returns a hash value for a given set item
// int eq(const void *, const void *) is a function that returns true if two set items are equal
// any two items that are equal must also have identical hash values
// capacity is the initial number of items the set can hold, which will be auto-increased as needed
BitWSet *BitWSetNew(size_t (*hash)(const void *), int (*eq)(const void *, const void *), size_t capacity);

// adds given item to set or replaces an equivalent existing item and returns item replaced if any
void *BitWSetAdd(BitWSet *set, void *item);

// removes item equivalent to given item from set and returns item removed if any
void *BitWSetRemove(BitWSet *set, const void *item);

// removes all items from set
void BitWSetClear(BitWSet *set);

// returns the number of items in set
size_t BitWSetCount(const BitWSet *set);

// true if an item equivalant to the given item is contained in set
int BitWSetContains(const BitWSet *set, const void *item);

// true if any items in otherSet are contained in set
int BitWSetIntersects(const BitWSet *set, const BitWSet *otherSet);

// returns member item from set equivalent to given item, or NULL if there is none
void *BitWSetGet(const BitWSet *set, const void *item);

// interates over set and returns the next item after previous, or NULL if no more items are available
// if previous is NULL, an initial item is returned
void *BitWSetIterate(const BitWSet *set, const void *previous);

// writes up to count items from set to allItems and returns number of items written
size_t BitWSetAll(const BitWSet *set, void *allItems[], size_t count);

// calls apply() with each item in set
void BitWSetApply(const BitWSet *set, void *info, void (*apply)(void *info, void *item));

// adds or replaces items from otherSet into set
void BitWSetUnion(BitWSet *set, const BitWSet *otherSet);

// removes items contained in otherSet from set
void BitWSetMinus(BitWSet *set, const BitWSet *otherSet);

// removes items not contained in otherSet from set
void BitWSetIntersect(BitWSet *set, const BitWSet *otherSet);

// frees memory allocated for set
void BitWSetFree(BitWSet *set);

#ifdef __cplusplus
}
#endif

#endif // BitWSet_h
