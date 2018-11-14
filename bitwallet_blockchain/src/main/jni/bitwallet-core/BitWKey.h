//
//  BitWKey.h
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

#ifndef BitWKey_h
#define BitWKey_h

#include "BitWInt.h"
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t p[33];
} BitWECPoint;

// adds 256bit big endian ints a and b (mod secp256k1 order) and stores the result in a
// returns true on success
int BitWSecp256k1ModAdd(UInt256 *a, const UInt256 *b);

// multiplies 256bit big endian ints a and b (mod secp256k1 order) and stores the result in a
// returns true on success
int BitWSecp256k1ModMul(UInt256 *a, const UInt256 *b);

// multiplies secp256k1 generator by 256bit big endian int i and stores the result in p
// returns true on success
int BitWSecp256k1PointGen(BitWECPoint *p, const UInt256 *i);

// multiplies secp256k1 generator by 256bit big endian int i and adds the result to ec-point p
// returns true on success
int BitWSecp256k1PointAdd(BitWECPoint *p, const UInt256 *i);

// multiplies secp256k1 ec-point p by 256bit big endian int i and stores the result in p
// returns true on success
int BitWSecp256k1PointMul(BitWECPoint *p, const UInt256 *i);

// returns true if privKey is a valid private key
// supported formats are wallet import format (WIF), mini private key format, or hex string
int BitWPrivKeyIsValid(const char *privKey, int coinType);

typedef struct {
    UInt256 secret;
    uint8_t pubKey[65];
    int compressed;
} BitWKey;

// assigns secret to key and returns true on success
int BitWKeySetSecret(BitWKey *key, const UInt256 *secret, int compressed);

// assigns privKey to key and returns true on success
// privKey must be wallet import format (WIF), mini private key format, or hex string
int BitWKeySetPrivKey(BitWKey *key, const char *privKey, int coinType);

// assigns DER encoded pubKey to key and returns true on success
int BitWKeySetPubKey(BitWKey *key, const uint8_t *pubKey, size_t pkLen);

// writes the WIF private key to privKey and returns the number of bytes writen, or pkLen needed if privKey is NULL
// returns 0 on failure
size_t BitWKeyPrivKey(const BitWKey *key, char *privKey, size_t pkLen, int coinType);

// writes the DER encoded public key to pubKey and returns number of bytes written, or pkLen needed if pubKey is NULL
size_t BitWKeyPubKey(BitWKey *key, void *pubKey, size_t pkLen);

// returns the ripemd160 hash of the sha256 hash of the public key, or UINT160_ZERO on error
UInt160 BitWKeyHash160(BitWKey *key);

// writes the pay-to-pubkey-hash bitcoin address for key to addr
// returns the number of bytes written, or addrLen needed if addr is NULL
size_t BitWKeyAddress(BitWKey *key, char *addr, size_t addrLen);
size_t BitWKeyAddressByCoinType(BitWKey *key, char *addr, size_t addrLen, int coinType);

// signs md with key and writes signature to sig
// returns the number of bytes written, or sigLen needed if sig is NULL
// returns 0 on failure
size_t BitWKeySign(const BitWKey *key, void *sig, size_t sigLen, UInt256 md);

// returns true if the signature for md is verified to have been made by key
int BitWKeyVerify(BitWKey *key, UInt256 md, const void *sig, size_t sigLen);

// wipes key material from key
void BitWKeyClean(BitWKey *key);

// Pieter Wuille's compact signature encoding used for bitcoin message signing
// to verify a compact signature, recover a public key from the signature and verify that it matches the signer's pubkey
size_t BitWKeyCompactSign(const BitWKey *key, void *compactSig, size_t sigLen, UInt256 md);

// assigns pubKey recovered from compactSig to key and returns true on success
int BitWKeyRecoverPubKey(BitWKey *key, UInt256 md, const void *compactSig, size_t sigLen);

uint8_t *uint8toHex(uint8_t *uint8Data, size_t scriptLen);

#ifdef __cplusplus
}
#endif

#endif // BitWKey_h
