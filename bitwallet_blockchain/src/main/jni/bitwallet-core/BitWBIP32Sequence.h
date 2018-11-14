//
//  BitWBIP32Sequence.h
//
//  Created by Aaron Voisine on 8/19/15.
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

#ifndef BitWBIP32Sequence_h
#define BitWBIP32Sequence_h

#include "BitWKey.h"
#include "BitWInt.h"
#include <stdarg.h>
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// BIP32 is a scheme for deriving chains of addresses from a seed value
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki

#define BIP32_HARD                  0x80000000

#define SEQUENCE_GAP_LIMIT_EXTERNAL 10
#define SEQUENCE_GAP_LIMIT_INTERNAL 5
#define SEQUENCE_EXTERNAL_CHAIN     0
#define SEQUENCE_INTERNAL_CHAIN     1

typedef struct {
    uint32_t fingerPrint;
    UInt256 chainCode;
    uint8_t pubKey[33];
} BitWMasterPubKey;

#define BitW_MASTER_PUBKEY_NONE ((BitWMasterPubKey) { 0, UINT256_ZERO, \
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } })

// returns the master public key for the default BIP32 wallet layout - derivation path N(m/0H)
BitWMasterPubKey BitWBIP32MasterPubKey(const void *seed, size_t seedLen);
// returns the master public key for the default BIP44 wallet layout - derivation path path N(m/44H/coinTypeH/0H)
BitWMasterPubKey BitWBIP44MasterPubKey(const void *seed, size_t seedLen, int coinType);

// writes the public key for path N(m/0H/chain/index) to pubKey
// returns number of bytes written, or pubKeyLen needed if pubKey is NULL
size_t BitWBIP32PubKey(uint8_t *pubKey, size_t pubKeyLen, BitWMasterPubKey mpk, uint32_t chain, uint32_t index);
size_t BitWBIP44PubKey(uint8_t *pubKey, size_t pubKeyLen, BitWMasterPubKey mpk, uint32_t chain, uint32_t index);

// sets the private key for path m/0H/chain/index to key
void BitWBIP32PrivKey(BitWKey *key, const void *seed, size_t seedLen, uint32_t chain, uint32_t index);

// sets the private key for path m/0H/chain/index to each element in keys
void BitWBIP32PrivKeyList(BitWKey keys[], size_t keysCount, const void *seed, size_t seedLen, uint32_t chain,
                        const uint32_t indexes[]);

void BitWBIP44PrivKeyList(int coinType, BitWKey keys[], size_t keysCount, const void *seed, size_t seedLen,
                          uint32_t chain,
                          const uint32_t indexes[]);

void BitWBIP32PubKeyPath(BitWMasterPubKey *mpk, const void *seed, size_t seedLen, int depth, ...);
// sets the private key for the specified path to key
// depth is the number of arguments used to specify the path
void BitWBIP32PrivKeyPath(BitWKey *key, const void *seed, size_t seedLen, int depth, ...);

// sets the private key for the path specified by vlist to key
// depth is the number of arguments in vlist
void BitWBIP32vPrivKeyPath(BitWKey *key, const void *seed, size_t seedLen, int depth, va_list vlist);

// writes the base58check encoded serialized master private key (xprv) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BitWBIP32SerializeMasterPrivKey(char *str, size_t strLen, const void *seed, size_t seedLen);

// writes a master private key to seed given a base58check encoded serialized master private key (xprv)
// returns number of bytes written, or seedLen needed if seed is NULL
size_t BitWBIP32ParseMasterPrivKey(void *seed, size_t seedLen, const char *str);

// writes the base58check encoded serialized master public key (xpub) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BitWBIP32SerializeMasterPubKey(char *str, size_t strLen, BitWMasterPubKey mpk);

// returns a master public key give a base58check encoded serialized master public key (xpub)
BitWMasterPubKey BitWBIP32ParseMasterPubKey(const char *str);

// key used for authenticated API calls, i.e. bitauth: https://github.com/bitpay/bitauth - path m/1H/0
void BitWBIP32APIAuthKey(BitWKey *key, const void *seed, size_t seedLen);

// key used for BitID: https://github.com/bitid/bitid/blob/master/BIP_draft.md
void BitWBIP32BitIDKey(BitWKey *key, const void *seed, size_t seedLen, uint32_t index, const char *uri);

void BitWBIP32FirstPrivKey(BitWKey *key, const void *seed, size_t seedLen);

void BitWBIP44PrivKey(BitWKey *key, const void *seed, size_t seedLen, int coinType, int chain, int index);

#ifdef __cplusplus
}
#endif

#endif // BitWBIP32Sequence_h
