//
//  BitWallet.h
//
//  Created by Aaron Voisine on 9/1/15.
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

#ifndef BitWallet_h
#define BitWallet_h

#include "BitWTransaction.h"
#include "BitWAddress.h"
#include "BitWBIP32Sequence.h"
#include "BitWInt.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FEE_PER_KB ((5000ULL*1000 + 99)/100) // bitcoind 0.11 min relay fee on 100bytes
#define MIN_FEE_PER_KB     ((TX_FEE_PER_KB*1000 + 190)/191) // minimum relay fee on a 191byte tx
#define MAX_FEE_PER_KB     ((1000100ULL*1000 + 190)/191) // slightly higher than a 10000bit fee on a 191byte tx

typedef struct {
    UInt256 hash;
    uint32_t n;
} BitWUTXO;

inline static size_t BitWUTXOHash(const void *utxo)
{
    // (hash xor n)*FNV_PRIME
    return (size_t)((((const BitWUTXO *)utxo)->hash.u32[0] ^ ((const BitWUTXO *)utxo)->n)*0x01000193);
}

inline static int BitWUTXOEq(const void *utxo, const void *otherUtxo)
{
    return (utxo == otherUtxo || (UInt256Eq(((const BitWUTXO *)utxo)->hash, ((const BitWUTXO *)otherUtxo)->hash) &&
                                  ((const BitWUTXO *)utxo)->n == ((const BitWUTXO *)otherUtxo)->n));
}

typedef struct BitWalletStruct BitWallet;

// allocates and populates a BitWallet struct that must be freed by calling BitWalletFree()
//BitWallet *BitWalletNew(BitWTransaction *transactions[], size_t txCount, BitWMasterPubKey mpk);
BitWallet *BitWalletNewByCoinType(BitWTransaction *transactions[], size_t txCount, BitWMasterPubKey mpk, int coinType);

// not thread-safe, set callbacks once after BitWalletNew(), before calling other BitWallet functions
// info is a void pointer that will be passed along with each callback call
// void balanceChanged(void *, uint64_t) - called when the wallet balance changes
// void txAdded(void *, BitWTransaction *) - called when transaction is added to the wallet
// void txUpdated(void *, const UInt256[], size_t, uint32_t, uint32_t)
//   - called when the blockHeight or timestamp of previously added transactions are updated
// void txDeleted(void *, UInt256, int, int) - called when a previously added transaction is removed from the wallet
void BitWalletSetCallbacks(BitWallet *wallet, void *info,
                          void (*balanceChanged)(void *info, uint64_t balance),
                          void (*txAdded)(void *info, BitWTransaction *tx),
                          void (*txUpdated)(void *info, const UInt256 txHashes[], size_t txCount, uint32_t blockHeight,
                                            uint32_t timestamp),
                          void (*txDeleted)(void *info, UInt256 txHash, int notifyUser, int recommendRescan));

// wallets are composed of chains of addresses
// each chain is traversed until a gap of a number of addresses is found that haven't been used in any transactions
// this function writes to addrs an array of <gapLimit> unused addresses following the last used address in the chain
// the internal chain is used for change addresses and the external chain for receive addresses
// addrs may be NULL to only generate addresses for BitWalletContainsAddress()
// returns the number addresses written to addrs
//size_t BitWalletUnusedAddrs(BitWallet *wallet, BitWAddress addrs[], uint32_t gapLimit, int internal);
size_t BitWalletUnusedAddrsByCoinType(BitWallet *wallet, BitWAddress addrs[], uint32_t gapLimit, int internal, int coinType);

// returns the first unused external address
BitWAddress BitWalletReceiveAddress(BitWallet *wallet);

// writes all addresses previously genereated with BitWalletUnusedAddrs() to addrs
// returns the number addresses written, or total number available if addrs is NULL
size_t BitWalletAllAddrs(BitWallet *wallet, BitWAddress addrs[], size_t addrsCount);

// true if the address was previously generated by BitWalletUnusedAddrs() (even if it's now used)
int BitWalletContainsAddress(BitWallet *wallet, const char *addr);

// true if the address was previously used as an input or output in any wallet transaction
int BitWalletAddressIsUsed(BitWallet *wallet, const char *addr);

// writes transactions registered in the wallet, sorted by date, oldest first, to the given transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BitWalletTransactions(BitWallet *wallet, BitWTransaction *transactions[], size_t txCount);

// writes transactions registered in the wallet, and that were unconfirmed before blockHeight, to the transactions array
// returns the number of transactions written, or total number available if transactions is NULL
size_t BitWalletTxUnconfirmedBefore(BitWallet *wallet, BitWTransaction *transactions[], size_t txCount,
                                   uint32_t blockHeight);

// current wallet balance, not including transactions known to be invalid
uint64_t BitWalletBalance(BitWallet *wallet);

// total amount spent from the wallet (exluding change)
uint64_t BitWalletTotalSent(BitWallet *wallet);

// total amount received by the wallet (exluding change)
uint64_t BitWalletTotalReceived(BitWallet *wallet);

// writes unspent outputs to utxos and returns the number of outputs written, or number available if utxos is NULL
size_t BitWalletUTXOs(BitWallet *wallet, BitWUTXO utxos[], size_t utxosCount);

// fee-per-kb of transaction size to use when creating a transaction
uint64_t BitWalletFeePerKb(BitWallet *wallet);
void BitWalletSetFeePerKb(BitWallet *wallet, uint64_t feePerKb);

// returns an unsigned transaction that sends the specified amount from the wallet to the given address
// result must be freed using BitWTransactionFree()
BitWTransaction *BitWalletCreateTransaction(BitWallet *wallet, uint64_t amount, const char *addr);

// returns an unsigned transaction that satisifes the given transaction outputs
// result must be freed using BitWTransactionFree()
BitWTransaction *BitWalletCreateTxForOutputs(BitWallet *wallet, const BitWTxOutput outputs[], size_t outCount);

// signs any inputs in tx that can be signed using private keys from the wallet
// forkId is 0 for bitcoin, 0x40 for b-cash
// seed is the master private key (wallet seed) corresponding to the master public key given when the wallet was created
// returns true if all inputs were signed, or false if there was an error or not all inputs were able to be signed
//int BitWalletSignTransaction(BitWallet *wallet,BitWTransaction *tx, int forkId, const void *seed, size_t seedLen);
int BitWalletSignTransactionByCoinType(BitWallet *wallet,int coinType, BitWTransaction *tx, int forkId, const void *seed, size_t seedLen);

// true if the given transaction is associated with the wallet (even if it hasn't been registered)
int BitWalletContainsTransaction(BitWallet *wallet, const BitWTransaction *tx);

// adds a transaction to the wallet, or returns false if it isn't associated with the wallet
int BitWalletRegisterTransaction(BitWallet *wallet, BitWTransaction *tx);

// removes a tx from the wallet and calls BitWTransactionFree() on it, along with any tx that depend on its outputs
void BitWalletRemoveTransaction(BitWallet *wallet, UInt256 txHash);

// returns the transaction with the given hash if it's been registered in the wallet
BitWTransaction *BitWalletTransactionForHash(BitWallet *wallet, UInt256 txHash);

// true if no previous wallet transaction spends any of the given transaction's inputs, and no inputs are invalid
int BitWalletTransactionIsValid(BitWallet *wallet, const BitWTransaction *tx);

// true if transaction cannot be immediately spent (i.e. if it or an input tx can be replaced-by-fee)
int BitWalletTransactionIsPending(BitWallet *wallet, const BitWTransaction *tx);

// true if tx is considered 0-conf safe (valid and not pending, timestamp is greater than 0, and no unverified inputs)
int BitWalletTransactionIsVerified(BitWallet *wallet, const BitWTransaction *tx);

// set the block heights and timestamps for the given transactions
// use height TX_UNCONFIRMED and timestamp 0 to indicate a tx should remain marked as unverified (not 0-conf safe)
void BitWalletUpdateTransactions(BitWallet *wallet, const UInt256 txHashes[], size_t txCount, uint32_t blockHeight,
                                uint32_t timestamp);
    
// marks all transactions confirmed after blockHeight as unconfirmed (useful for chain re-orgs)
void BitWalletSetTxUnconfirmedAfter(BitWallet *wallet, uint32_t blockHeight);

// returns the amount received by the wallet from the transaction (total outputs to change and/or receive addresses)
uint64_t BitWalletAmountReceivedFromTx(BitWallet *wallet, const BitWTransaction *tx);

// returns the amount sent from the wallet by the trasaction (total wallet outputs consumed, change and fee included)
uint64_t BitWalletAmountSentByTx(BitWallet *wallet, const BitWTransaction *tx);

// returns the fee for the given transaction if all its inputs are from wallet transactions, UINT64_MAX otherwise
uint64_t BitWalletFeeForTx(BitWallet *wallet, const BitWTransaction *tx);

// historical wallet balance after the given transaction, or current balance if transaction is not registered in wallet
uint64_t BitWalletBalanceAfterTx(BitWallet *wallet, const BitWTransaction *tx);

// fee that will be added for a transaction of the given size in bytes
uint64_t BitWalletFeeForTxSize(BitWallet *wallet, size_t size);

// fee that will be added for a transaction of the given amount
uint64_t BitWalletFeeForTxAmount(BitWallet *wallet, uint64_t amount);

// outputs below this amount are uneconomical due to fees (TX_MIN_OUTPUT_AMOUNT is the absolute minimum output amount)
uint64_t BitWalletMinOutputAmount(BitWallet *wallet);

// maximum amount that can be sent from the wallet to a single address after fees
uint64_t BitWalletMaxOutputAmount(BitWallet *wallet);

// frees memory allocated for wallet, and calls BitWTransactionFree() for all registered transactions
void BitWalletFree(BitWallet *wallet, int coinType);

// returns the given amount (in satoshis) in local currency units (i.e. pennies, pence)
// price is local currency units per bitcoin
int64_t BitWLocalAmount(int64_t amount, double price);

// returns the given local currency amount in satoshis
// price is local currency units (i.e. pennies, pence) per bitcoin
int64_t BitWBitcoinAmount(int64_t localAmount, double price);

#ifdef __cplusplus
}
#endif

#endif // BitWallet_h
