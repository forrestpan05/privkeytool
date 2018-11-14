//
//  BitWAddress.h
//
//  Created by Aaron Voisine on 9/18/15.
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

#ifndef BitWAddress_h
#define BitWAddress_h

#include "BitWCrypto.h"
#include <string.h>
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if BITCOIN_TESTNET
#pragma message "testnet build"
#endif

#define BIP44_COIN_TYPE_BTC     0
#define BIP44_COIN_TYPE_BCH     145
#define BIP44_COIN_TYPE_BTN     1000
#define BIP44_COIN_TYPE_ETH     60
#define BIP44_COIN_TYPE_EOS     194
#define BIP44_COIN_TYPE_QTUM    2301


#define BITCOIN_PRIVKEY      128 //0x80
#define BITCOIN_PRIVKEY_TEST 239

// bitcoin address prefixes
#define BITCOIN_PUBKEY_ADDRESS      0
#define BITCOIN_SCRIPT_ADDRESS      5
#define BITCOIN_PUBKEY_ADDRESS_TEST 111
#define BITCOIN_SCRIPT_ADDRESS_TEST 196

//QTUM地址
#define QTUM_PUBKEY_ADDRESS      58
#define QTUM_SCRIPT_ADDRESS      50

#define QTUM_PUBKEY_ADDRESS_TEST 120
#define QTUM_SCRIPT_ADDRESS_TEST 110

// bitcoin script opcodes: https://en.bitcoin.it/wiki/Script#Constants
#define OP_0           0x00
#define OP_PUSHDATA1   0x4c
#define OP_PUSHDATA2   0x4d
#define OP_PUSHDATA4   0x4e
#define OP_DUP         0x76
#define OP_EQUAL       0x87
#define OP_EQUALVERIFY 0x88
#define OP_HASH160     0xa9
#define OP_CHECKSIG    0xac

// reads a varint from buf and stores its length in intLen if intLen is non-NULL
// returns the varint value
uint64_t BitWVarInt(const uint8_t *buf, size_t bufLen, size_t *intLen);

// writes i to buf as a varint and returns the number of bytes written, or bufLen needed if buf is NULL
size_t BitWVarIntSet(uint8_t *buf, size_t bufLen, uint64_t i);

// returns the number of bytes needed to encode i as a varint
size_t BitWVarIntSize(uint64_t i);

// parses script and writes an array of pointers to the script elements (opcodes and data pushes) to elems
// returns the number of elements written, or elemsCount needed if elems is NULL
size_t BitWScriptElements(const uint8_t *elems[], size_t elemsCount, const uint8_t *script, size_t scriptLen);

// given a data push script element, returns a pointer to the start of the data and writes its length to dataLen
const uint8_t *BitWScriptData(const uint8_t *elem, size_t *dataLen);

// writes a data push script element to script
// returns the number of bytes written, or scriptLen needed if script is NULL
size_t BitWScriptPushData(uint8_t *script, size_t scriptLen, const uint8_t *data, size_t dataLen);

typedef struct {
    char s[36];
} BitWAddress;

#define BitW_ADDRESS_NONE ((BitWAddress) { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" })

// writes the bitcoin address for a scriptPubKey to addr
// returns the number of bytes written, or addrLen needed if addr is NULL
//size_t BitWAddressFromScriptPubKey(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen);
size_t BitWAddressFromScriptPubKeyByCoinType(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen, int coinType);

// writes the bitcoin address for a scriptSig to addr
// returns the number of bytes written, or addrLen needed if addr is NULL
//size_t BitWAddressFromScriptSig(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen);
size_t BitWAddressFromScriptSigByCoinType(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen, int coinType);
// writes the scriptPubKey for addr to script
// returns the number of bytes written, or scriptLen needed if script is NULL
//size_t BitWAddressScriptPubKey(uint8_t *script, size_t scriptLen, const char *addr);
size_t BitWAddressScriptPubKeyByCoinType(uint8_t *script, size_t scriptLen, const char *addr, int coinType);

// returns true if addr is a valid bitcoin address
//int BitWAddressIsValid(const char *addr);
// returns true if addr is a valid bitcoin address
int BitWAddressIsValidByCoinType(const char *addr, int coinType);

// writes the 20 byte hash160 of addr to md20 and returns true on success
int BitWAddressHash160(void *md20, const char *addr);

// returns a hash value for addr suitable for use in a hashtable
inline static size_t BitWAddressHash(const void *addr)
{
    return BitWMurmur3_32(addr, strlen((const char *)addr), 0);
}

// true if addr and otherAddr are equal
inline static int BitWAddressEq(const void *addr, const void *otherAddr)
{
    return (addr == otherAddr ||
            strncmp((const char *)addr, (const char *)otherAddr, sizeof(BitWAddress)) == 0);
}

#ifdef __cplusplus
}
#endif

#endif // BitWAddress_h
