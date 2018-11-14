//
//  BitWBIP32Sequence.c
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

#include "BitWBIP32Sequence.h"
#include "BitWCrypto.h"
#include "BitWBase58.h"
#include "BitWKey.h"
#include <string.h>
#include <assert.h>

#define BIP32_SEED_KEY "Bitcoin seed"
#define BIP32_XPRV     "\x04\x88\xAD\xE4"
#define BIP32_XPUB     "\x04\x88\xB2\x1E"

// BIP32 is a scheme for deriving chains of addresses from a seed value
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki

// Private parent key -> private child key
//
// CKDpriv((kpar, cpar), i) -> (ki, ci) computes a child extended private key from the parent extended private key:
//
// - Check whether i >= 2^31 (whether the child is a hardened key).
//     - If so (hardened child): let I = HMAC-SHA512(Key = cpar, Data = 0x00 || ser256(kpar) || ser32(i)).
//       (Note: The 0x00 pads the private key to make it 33 bytes long.)
//     - If not (normal child): let I = HMAC-SHA512(Key = cpar, Data = serP(point(kpar)) || ser32(i)).
// - Split I into two 32-byte sequences, IL and IR.
// - The returned child key ki is parse256(IL) + kpar (mod n).
// - The returned chain code ci is IR.
// - In case parse256(IL) >= n or ki = 0, the resulting key is invalid, and one should proceed with the next value for i
//   (Note: this has probability lower than 1 in 2^127.)
//
static void _CKDpriv(UInt256 *k, UInt256 *c, uint32_t i) {
    uint8_t buf[sizeof(BitWECPoint) + sizeof(i)];
    UInt512 I;

    if (i & BIP32_HARD) {
        buf[0] = 0;
        UInt256Set(&buf[1], *k);
    } else BitWSecp256k1PointGen((BitWECPoint *) buf, k);

    UInt32SetBE(&buf[sizeof(BitWECPoint)], i);

    BitWHMAC(&I, BitWSHA512, sizeof(UInt512), c, sizeof(*c), buf,
             sizeof(buf)); // I = HMAC-SHA512(c, k|P(k) || i)

    BitWSecp256k1ModAdd(k, (UInt256 *) &I); // k = IL + k (mod n)
    *c = *(UInt256 *) &I.u8[sizeof(UInt256)]; // c = IR

    var_clean(&I);
    mem_clean(buf, sizeof(buf));
}

// Public parent key -> public child key
//
// CKDpub((Kpar, cpar), i) -> (Ki, ci) computes a child extended public key from the parent extended public key.
// It is only defined for non-hardened child keys.
//
// - Check whether i >= 2^31 (whether the child is a hardened key).
//     - If so (hardened child): return failure
//     - If not (normal child): let I = HMAC-SHA512(Key = cpar, Data = serP(Kpar) || ser32(i)).
// - Split I into two 32-byte sequences, IL and IR.
// - The returned child key Ki is point(parse256(IL)) + Kpar.
// - The returned chain code ci is IR.
// - In case parse256(IL) >= n or Ki is the point at infinity, the resulting key is invalid, and one should proceed with
//   the next value for i.
//
static void _CKDpub(BitWECPoint *K, UInt256 *c, uint32_t i) {
    uint8_t buf[sizeof(*K) + sizeof(i)];
    UInt512 I;

    if ((i & BIP32_HARD) != BIP32_HARD) { // can't derive private child key from public parent key
        *(BitWECPoint *) buf = *K;
        UInt32SetBE(&buf[sizeof(*K)], i);

        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), c, sizeof(*c), buf,
                 sizeof(buf)); // I = HMAC-SHA512(c, P(K) || i)

        *c = *(UInt256 *) &I.u8[sizeof(UInt256)]; // c = IR
        BitWSecp256k1PointAdd(K, (UInt256 *) &I); // K = P(IL) + K

        var_clean(&I);
        mem_clean(buf, sizeof(buf));
    }
}

// returns the master public key for the default BIP32 wallet layout - derivation path N(m/0H)
BitWMasterPubKey BitWBIP32MasterPubKey(const void *seed, size_t seedLen) {
    BitWMasterPubKey mpk = BitW_MASTER_PUBKEY_NONE;
    UInt512 I;
    UInt256 secret, chain;
    BitWKey key;

    assert(seed != NULL || seedLen == 0);

    if (seed || seedLen == 0) {
        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed,
                 seedLen);
        secret = *(UInt256 *) &I;
        chain = *(UInt256 *) &I.u8[sizeof(UInt256)];
        var_clean(&I);

        BitWKeySetSecret(&key, &secret, 1);
        mpk.fingerPrint = BitWKeyHash160(&key).u32[0];

      _CKDpriv(&secret, &chain, 0 | BIP32_HARD);    //path m/0H

        mpk.chainCode = chain;
        BitWKeySetSecret(&key, &secret, 1);

        var_clean(&secret, &chain);
        BitWKeyPubKey(&key, &mpk.pubKey, sizeof(mpk.pubKey)); // path N(m/0H)
        BitWKeyClean(&key);
    }

    return mpk;
}

/**
 * m / purpose' / coin_type' / account' / change / address_index
 * @param seed
 * @param seedLen
 * @return
 */
BitWMasterPubKey BitWBIP44MasterPubKey(const void *seed, size_t seedLen, int coinType) {
    BitWMasterPubKey mpk = BitW_MASTER_PUBKEY_NONE;
    BitWBIP32PubKeyPath(&mpk, seed, seedLen, 3, 44 | BIP32_HARD, coinType | BIP32_HARD, 0 | BIP32_HARD);
    return mpk;
}


void BitWBIP32vPubKeyPath(BitWMasterPubKey *mpk, const void *seed, size_t seedLen, int depth, va_list vlist) {
    UInt512 I;
    UInt256 secret, chainCode;
    BitWKey key;

    assert(mpk != NULL);
    assert(seed != NULL || seedLen == 0);
    assert(depth >= 0);

    if (mpk && (seed || seedLen == 0)) {
        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed,
                 seedLen);
        secret = *(UInt256 *) &I;
        chainCode = *(UInt256 *) &I.u8[sizeof(UInt256)];
        var_clean(&I);

        BitWKeySetSecret(&key, &secret, 1);
        mpk->fingerPrint = BitWKeyHash160(&key).u32[0];

        for (int i = 0; i < depth; i++) {
            _CKDpriv(&secret, &chainCode, va_arg(vlist, uint32_t));
        }
        mpk->chainCode = chainCode;
        BitWKeySetSecret(&key, &secret, 1);
        var_clean(&secret, &chainCode);

        BitWKeyPubKey(&key, &mpk->pubKey, sizeof(mpk->pubKey)); // path
        BitWKeyClean(&key);
    }
}

void BitWBIP32PubKeyPath(BitWMasterPubKey *mpk, const void *seed, size_t seedLen, int depth, ...) {
    va_list ap;
    va_start(ap, depth);
    BitWBIP32vPubKeyPath(mpk, seed, seedLen, depth, ap);
    va_end(ap);
}



// writes the public key for path N(m/0H/chain/index) to pubKey
// returns number of bytes written, or pubKeyLen needed if pubKey is NULL
size_t BitWBIP32PubKey(uint8_t *pubKey, size_t pubKeyLen, BitWMasterPubKey mpk, uint32_t chain,
                       uint32_t index) {
    UInt256 chainCode = mpk.chainCode;

    assert(memcmp(&mpk, &BitW_MASTER_PUBKEY_NONE, sizeof(mpk)) != 0);

    if (pubKey && sizeof(BitWECPoint) <= pubKeyLen) {
        *(BitWECPoint *) pubKey = *(BitWECPoint *) mpk.pubKey;

        _CKDpub((BitWECPoint *) pubKey, &chainCode, chain); // path N(m/0H/chain)
        _CKDpub((BitWECPoint *) pubKey, &chainCode, index); // index'th key in chain
        var_clean(&chainCode);
    }

    return (!pubKey || sizeof(BitWECPoint) <= pubKeyLen) ? sizeof(BitWECPoint) : 0;
}
// writes the public key for path N(m/44H/coinTypeH/0H/chain/index) to pubKey
// returns number of bytes written, or pubKeyLen needed if pubKey is NULL
size_t BitWBIP44PubKey(uint8_t *pubKey, size_t pubKeyLen, BitWMasterPubKey mpk, uint32_t chain,
                       uint32_t index)
{
    return BitWBIP32PubKey(pubKey, pubKeyLen, mpk, chain, index);
}

// sets the private key for path m/0H/chain/index to key
void
BitWBIP32PrivKey(BitWKey *key, const void *seed, size_t seedLen, uint32_t chain, uint32_t index) {
    BitWBIP32PrivKeyPath(key, seed, seedLen, 3, 0 | BIP32_HARD, chain, index);
}

// sets the private key for path m/0H/chain/index to each element in keys
void BitWBIP32PrivKeyList(BitWKey keys[], size_t keysCount, const void *seed, size_t seedLen,
                          uint32_t chain,
                          const uint32_t indexes[]) {
    UInt512 I;
    UInt256 secret, chainCode, s, c;

    assert(keys != NULL || keysCount == 0);
    assert(seed != NULL || seedLen == 0);
    assert(indexes != NULL || keysCount == 0);

    if (keys && keysCount > 0 && (seed || seedLen == 0) && indexes) {
        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed,
                 seedLen);
        secret = *(UInt256 *) &I;
        chainCode = *(UInt256 *) &I.u8[sizeof(UInt256)];
        var_clean(&I);

        _CKDpriv(&secret, &chainCode, 0 | BIP32_HARD); // path m/0H
        _CKDpriv(&secret, &chainCode, chain); // path m/0H/chain

        for (size_t i = 0; i < keysCount; i++) {
            s = secret;
            c = chainCode;
            _CKDpriv(&s, &c, indexes[i]); // index'th key in chain
            BitWKeySetSecret(&keys[i], &s, 1);
        }

        var_clean(&secret, &chainCode, &c, &s);
    }
}

void BitWBIP44PrivKeyList(int coinType, BitWKey keys[], size_t keysCount, const void *seed, size_t seedLen,
                          uint32_t chain,
                          const uint32_t indexes[]) {
    UInt512 I;
    UInt256 secret, chainCode, s, c;

    assert(keys != NULL || keysCount == 0);
    assert(seed != NULL || seedLen == 0);
    assert(indexes != NULL || keysCount == 0);

    if (keys && keysCount > 0 && (seed || seedLen == 0) && indexes) {
        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed,
                 seedLen);
        secret = *(UInt256 *) &I;
        chainCode = *(UInt256 *) &I.u8[sizeof(UInt256)];
        var_clean(&I);
        
        _CKDpriv(&secret, &chainCode, 44 | BIP32_HARD); // path m/44H
        _CKDpriv(&secret, &chainCode, coinType | BIP32_HARD); // path m/44H/cointype H
        _CKDpriv(&secret, &chainCode, 0|BIP32_HARD); // path m/44H/cointype H/0H
        _CKDpriv(&secret, &chainCode, chain); // path m/44H/cointype H/0H/chain

        for (size_t i = 0; i < keysCount; i++) {
            s = secret;
            c = chainCode;
            _CKDpriv(&s, &c, indexes[i]); // index'th key in chain
            BitWKeySetSecret(&keys[i], &s, 1);
        }

        var_clean(&secret, &chainCode, &c, &s);
    }
}
// sets the private key for the specified path to key
// depth is the number of arguments used to specify the path
void BitWBIP32PrivKeyPath(BitWKey *key, const void *seed, size_t seedLen, int depth, ...) {
    va_list ap;

    va_start(ap, depth);
    BitWBIP32vPrivKeyPath(key, seed, seedLen, depth, ap);
    va_end(ap);
}

// sets the private key for the path specified by vlist to key
// depth is the number of arguments in vlist
void
BitWBIP32vPrivKeyPath(BitWKey *key, const void *seed, size_t seedLen, int depth, va_list vlist) {
    UInt512 I;
    UInt256 secret, chainCode;

    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);
    assert(depth >= 0);

    if (key && (seed || seedLen == 0)) {
        BitWHMAC(&I, BitWSHA512, sizeof(UInt512), BIP32_SEED_KEY, strlen(BIP32_SEED_KEY), seed,
                 seedLen);
        secret = *(UInt256 *) &I;
        chainCode = *(UInt256 *) &I.u8[sizeof(UInt256)];
        var_clean(&I);

        for (int i = 0; i < depth; i++) {
            _CKDpriv(&secret, &chainCode, va_arg(vlist, uint32_t));
        }

        BitWKeySetSecret(key, &secret, 1);
        var_clean(&secret, &chainCode);
    }
}

// writes the base58check encoded serialized master private key (xprv) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BitWBIP32SerializeMasterPrivKey(char *str, size_t strLen, const void *seed, size_t seedLen) {
    // TODO: XXX implement
    return 0;
}

// writes a master private key to seed given a base58check encoded serialized master private key (xprv)
// returns number of bytes written, or seedLen needed if seed is NULL
size_t BitWBIP32ParseMasterPrivKey(void *seed, size_t seedLen, const char *str) {
    // TODO: XXX implement
    return 0;
}

// writes the base58check encoded serialized master public key (xpub) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BitWBIP32SerializeMasterPubKey(char *str, size_t strLen, BitWMasterPubKey mpk) {
    // TODO: XXX implement
    return 0;
}

// returns a master public key give a base58check encoded serialized master public key (xpub)
BitWMasterPubKey BitWBIP32ParseMasterPubKey(const char *str) {
    // TODO: XXX implement
    return BitW_MASTER_PUBKEY_NONE;
}

// key used for authenticated API calls, i.e. bitauth: https://github.com/bitpay/bitauth - path m/1H/0
void BitWBIP32APIAuthKey(BitWKey *key, const void *seed, size_t seedLen) {
    BitWBIP32PrivKeyPath(key, seed, seedLen, 2, 1 | BIP32_HARD, 0);
}

// key used for BitID: https://github.com/bitid/bitid/blob/master/BIP_draft.md
void
BitWBIP32BitIDKey(BitWKey *key, const void *seed, size_t seedLen, uint32_t index, const char *uri) {
    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);
    assert(uri != NULL);

    if (key && (seed || seedLen == 0) && uri) {
        UInt256 hash;
        size_t uriLen = strlen(uri);
        uint8_t data[sizeof(index) + uriLen];

        UInt32SetLE(data, index);
        memcpy(&data[sizeof(index)], uri, uriLen);
        BitWSHA256(&hash, data, sizeof(data));
        BitWBIP32PrivKeyPath(key, seed, seedLen,
                             5,
                             13 | BIP32_HARD,
                             UInt32GetLE(&hash.u32[0]) | BIP32_HARD,
                             UInt32GetLE(&hash.u32[1]) | BIP32_HARD,
                             UInt32GetLE(&hash.u32[2]) | BIP32_HARD,
                             UInt32GetLE(&hash.u32[3]) | BIP32_HARD); // path m/13H/aH/bH/cH/dH
    }
}

/**
 * 获取固定路径的key m/0H/chain/index
 * @param key
 * @param seed
 * @param seedLen
 */
void BitWBIP32FirstPrivKey(BitWKey *key, const void *seed, size_t seedLen) {
    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);

    if (key && (seed || seedLen == 0)) {
        BitWBIP32PrivKey(key, seed, seedLen, SEQUENCE_EXTERNAL_CHAIN, 0);
    }
}

void BitWBIP44PrivKey(BitWKey *key, const void *seed, size_t seedLen, int coinType, int change, int index) {
    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);

    if (key && (seed || seedLen == 0)) {
        BitWBIP32PrivKeyPath(key, seed, seedLen, 5, 44 | BIP32_HARD, coinType | BIP32_HARD, 0 | BIP32_HARD, change, index);
    }
}


