//
//  BitWMerkleBlock.h
//
//  Created by Aaron Voisine on 8/6/15.
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

#ifndef BitWMerkleBlock_h
#define BitWMerkleBlock_h

#include "BitWInt.h"
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_DIFFICULTY_INTERVAL 2016 // number of blocks between difficulty target adjustments
#define BLOCK_UNKNOWN_HEIGHT      INT32_MAX
#define BLOCK_MAX_TIME_DRIFT      (2*60*60) // the furthest in the future a block is allowed to be timestamped

typedef struct {
    UInt256 blockHash;
    uint32_t version;
    UInt256 prevBlock;
    UInt256 merkleRoot;
    uint32_t timestamp; // time interval since unix epoch
    uint32_t target;
    uint32_t nonce;
    uint32_t totalTx;
    UInt256 *hashes;
    size_t hashesCount;
    uint8_t *flags;
    size_t flagsLen;
    uint32_t height;
} BitWMerkleBlock;

#define BitW_MERKLE_BLOCK_NONE\
    ((BitWMerkleBlock) { UINT256_ZERO, 0, UINT256_ZERO, UINT256_ZERO, 0, 0, 0, 0, NULL, 0, NULL, 0, 0 })

// returns a newly allocated merkle block struct that must be freed by calling BitWMerkleBlockFree()
BitWMerkleBlock *BitWMerkleBlockNew(void);

// buf must contain either a serialized merkleblock or header
// returns a merkle block struct that must be freed by calling BitWMerkleBlockFree()
BitWMerkleBlock *BitWMerkleBlockParse(const uint8_t *buf, size_t bufLen);

// returns number of bytes written to buf, or total bufLen needed if buf is NULL (block->height is not serialized)
size_t BitWMerkleBlockSerialize(const BitWMerkleBlock *block, uint8_t *buf, size_t bufLen);

// populates txHashes with the matched tx hashes in the block
// returns number of tx hashes written, or the total hashesCount needed if txHashes is NULL
size_t BitWMerkleBlockTxHashes(const BitWMerkleBlock *block, UInt256 *txHashes, size_t hashesCount);

// sets the hashes and flags fields for a block created with BitWMerkleBlockNew()
void BitWMerkleBlockSetTxHashes(BitWMerkleBlock *block, const UInt256 hashes[], size_t hashesCount,
                              const uint8_t *flags, size_t flagsLen);

// true if merkle tree and timestamp are valid, and proof-of-work matches the stated difficulty target
// NOTE: this only checks if the block difficulty matches the difficulty target in the header, it does not check if the
// target is correct for the block's height in the chain - use BitWMerkleBlockVerifyDifficulty() for that
int BitWMerkleBlockIsValid(const BitWMerkleBlock *block, uint32_t currentTime);

// true if the given tx hash is known to be included in the block
int BitWMerkleBlockContainsTxHash(const BitWMerkleBlock *block, UInt256 txHash);

// verifies the block difficulty target is correct for the block's position in the chain
// transitionTime is the timestamp of the block at the previous difficulty transition
// transitionTime may be 0 if block->height is not a multiple of BLOCK_DIFFICULTY_INTERVAL
int BitWMerkleBlockVerifyDifficulty(const BitWMerkleBlock *block, const BitWMerkleBlock *previous, uint32_t transitionTime);

// returns a hash value for block suitable for use in a hashtable
inline static size_t BitWMerkleBlockHash(const void *block)
{
    return (size_t)((const BitWMerkleBlock *)block)->blockHash.u32[0];
}

// true if block and otherBlock have equal blockHash values
inline static int BitWMerkleBlockEq(const void *block, const void *otherBlock)
{
    return (block == otherBlock ||
            UInt256Eq(((const BitWMerkleBlock *)block)->blockHash, ((const BitWMerkleBlock *)otherBlock)->blockHash));
}

// frees memory allocated for block
void BitWMerkleBlockFree(BitWMerkleBlock *block);

#ifdef __cplusplus
}
#endif

#endif // BitWMerkleBlock_h
