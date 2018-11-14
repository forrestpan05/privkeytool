//
//  wallet.c
//
//  Created by Mihail Gutan on 12/4/15.
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

#include "wallet.h"
#include "PeerManager.h"
#include "BitWPeerManager.h"
#include "BitWBIP39Mnemonic.h"
#include "BitWBase58.h"
#include <assert.h>
#include <BitWBIP38Key.h>
#include <BitWInt.h>
#include <BitWTransaction.h>
#include <BitWKey.h>
#include <BitWAddress.h>
#include <stdio.h>
#include <BitWBIP32Sequence.h>
#include <BitWPeer.h>

static JavaVM *_jvmW;
BitWallet *_wallet;
static BitWTransaction **_transactions;
static BitWTransaction *_privKeyTx;
static uint64_t _privKeyBalance;
static size_t _transactionsCounter = 0;
jclass _walletManagerClass;

//#if BITCOIN_TESTNET
//#error don't know bcash testnet fork height
//#else // mainnet
#define BCASH_FORKHEIGHT 478559
//#endif


static JNIEnv *getEnv() {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getEnv Wallet");
    if (!_jvmW) return NULL;

    JNIEnv *env;
    int status = (*_jvmW)->GetEnv(_jvmW, (void **) &env, JNI_VERSION_1_6);

    if (status < 0) {
        status = (*_jvmW)->AttachCurrentThread(_jvmW, &env, NULL);
        if (status < 0) return NULL;
    }

    return env;
}

//callback for tx publishing
void callback(void *info, int error) {
    __android_log_print(ANDROID_LOG_ERROR, "Message from callback: ", "err: %s",
                        strerror(error));
    BitWTransaction *tx = info;
//    tx.
    JNIEnv *env = getEnv();

    if (!env || _walletManagerClass == NULL) return;

    jmethodID mid = (*env)->GetStaticMethodID(env, _walletManagerClass, "publishCallback",
                                              "(Ljava/lang/String;I[B)V");
    //call java methods
    if (error) {
        __android_log_print(ANDROID_LOG_ERROR, "Message from callback: ", "publishing Failed: %s",
                            strerror(error));
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "Message from callback: ", "publishing Succeeded!");
    }
    UInt256 txid = tx->txHash;
    jbyteArray JtxHash = (*env)->NewByteArray(env, sizeof(txid));
    (*env)->SetByteArrayRegion(env, JtxHash, 0, (jsize) sizeof(txid), (jbyte *) txid.u8);

    (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid,
                                 (*env)->NewStringUTF(env, strerror(error)),
                                 error, JtxHash);
}

static void balanceChanged(void *info, uint64_t balance) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "balanceChanged: %d", (int) balance);
    JNIEnv *env = getEnv();

    if (!env || _walletManagerClass == NULL) return;

    jmethodID mid = (*env)->GetStaticMethodID(env, _walletManagerClass, "onBalanceChanged", "(J)V");

    //call java methods
    (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid, balance);
}

static void txAdded(void *info, BitWTransaction *tx) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "txAdded");
    if (!_wallet || !tx) return;

    JNIEnv *env = getEnv();

    if (!env || _walletManagerClass == NULL) return;

    jmethodID mid = (*env)->GetStaticMethodID(env, _walletManagerClass, "onTxAdded",
                                              "([BIJJLjava/lang/String;)V");

    //call java methods
//    __android_log_print(ANDROID_LOG_ERROR, "Message from C: ",
//                        "BitWPeerManagerLastBlockHeight(): %d tx->timestamp: %d",
//                        tx->blockHeight, tx->timestamp);

    uint8_t buf[BitWTransactionSerialize(tx, NULL, 0)];
    size_t len = BitWTransactionSerialize(tx, buf, sizeof(buf));
    uint64_t fee = (BitWalletFeeForTx(_wallet, tx) == -1) ? 0 : BitWalletFeeForTx(_wallet, tx);
    jlong amount;

//    __android_log_print(ANDROID_LOG_ERROR, "Message from C: ", "fee: %d", (int)fee);
    if (BitWalletAmountSentByTx(_wallet, tx) == 0) {
        amount = (jlong) BitWalletAmountReceivedFromTx(_wallet, tx);
    } else {
        amount = (jlong) (
                (BitWalletAmountSentByTx(_wallet, tx) - BitWalletAmountReceivedFromTx(_wallet, tx) -
                 fee) * -1);
    }

    jbyteArray result = (*env)->NewByteArray(env, (jsize) len);

    (*env)->SetByteArrayRegion(env, result, 0, (jsize) len, (jbyte *) buf);

    UInt256 transactionHash = tx->txHash;
    const char *strHash = u256_hex_encode(transactionHash);
    jstring jstrHash = (*env)->NewStringUTF(env, strHash);

    (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid, result, (jint) tx->blockHeight,
                                 (jlong) tx->timestamp,
                                 (jlong) amount, jstrHash);
    (*env)->DeleteLocalRef(env, jstrHash);
}

static void txUpdated(void *info, const UInt256 txHashes[], size_t count, uint32_t blockHeight,
                      uint32_t timestamp) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "txUpdated");
    if (!_wallet) return;

    JNIEnv *env = getEnv();

    if (!env || _walletManagerClass == NULL) return;

    jmethodID mid = (*env)->GetStaticMethodID(env, _walletManagerClass, "onTxUpdated",
                                              "(Ljava/lang/String;II)V");

    for (size_t i = 0; i < count; i++) {
        const char *strHash = u256_hex_encode(txHashes[i]);
        jstring JstrHash = (*env)->NewStringUTF(env, strHash);

        (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid, JstrHash, (jint) blockHeight,
                                     (jint) timestamp);
        (*env)->DeleteLocalRef(env, JstrHash);
    }
}

static void txDeleted(void *info, UInt256 txHash, int notifyUser, int recommendRescan) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "txDeleted");
    if (!_wallet) return;

    JNIEnv *env = getEnv();

    if (!env || _walletManagerClass == NULL) return;

    const char *strHash = u256_hex_encode(txHash);

    //create class
    jmethodID mid = (*env)->GetStaticMethodID(env, _walletManagerClass, "onTxDeleted",
                                              "(Ljava/lang/String;II)V");
//    //call java methods
    (*env)->CallStaticVoidMethod(env, _walletManagerClass, mid, (*env)->NewStringUTF(env, strHash));
}


JNIEXPORT jbyteArray
Java_com_jniwrappers_MnemonicCode_encodeSeed(JNIEnv *env, jobject thiz,
                                             jbyteArray seed,
                                             jobjectArray stringArray) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "encodeSeed");

    int wordsCount = (*env)->GetArrayLength(env, stringArray);
    int seedLength = (*env)->GetArrayLength(env, seed);
    const char *wordList[wordsCount];

    for (int i = 0; i < wordsCount; i++) {
        jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
        const char *rawString = (*env)->GetStringUTFChars(env, string, 0);

        wordList[i] = rawString;
        (*env)->DeleteLocalRef(env, string);
    }

    jbyte *byteSeed = (*env)->GetByteArrayElements(env, seed, 0);
    char result[BitWBIP39Encode(NULL, 0, wordList, (uint8_t *) byteSeed, (size_t) seedLength)];

    BitWBIP39Encode((char *) result, sizeof(result), wordList, (const uint8_t *) byteSeed,
                    (size_t) seedLength);
    jbyte *phraseJbyte = (jbyte *) result;
    int size = sizeof(result) - 1;
    jbyteArray bytePhrase = (*env)->NewByteArray(env, size);
    (*env)->SetByteArrayRegion(env, bytePhrase, 0, size, phraseJbyte);

    return bytePhrase;
}

JNIEXPORT void
Java_com_jniwrappers_BitWallet_createWallet(JNIEnv *env, jobject thiz,
                                            size_t txCount,
                                            jbyteArray bytePubKey) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "createWallet");

    jint rs = (*env)->GetJavaVM(env, &_jvmW); // cache the JavaVM pointer
    jclass peerManagerCLass = (*env)->FindClass(env, "com/bitwallet/blockchain/BitWalletManager");
    _walletManagerClass = (jclass) (*env)->NewGlobalRef(env, (jobject) peerManagerCLass);

    if (_wallet) return;

    jbyte *pubKeyBytes = (*env)->GetByteArrayElements(env, bytePubKey, 0);
    BitWMasterPubKey pubKey = *(BitWMasterPubKey *) pubKeyBytes;

    if (rs != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "Message from C: ",
                            "WARNING, GetJavaVM is not JNI_OK");
    }

    if (!_transactions) {
        __android_log_print(ANDROID_LOG_ERROR, "Message from C: ",
                            "WARNING, _transactions is NULL, txCount: %zu",
                            txCount);
        txCount = 0;
    }

    BitWallet *w;

    if (txCount > 0) {
        __android_log_print(ANDROID_LOG_ERROR, "Message from C: ",
                            "BitWalletNew with tx nr: %zu", sizeof(_transactions));
        w = BitWalletNewByCoinType(_transactions, txCount, pubKey, BIP44_COIN_TYPE_BTC);
        _transactionsCounter = 0;

        if (_transactions) {
            free(_transactions);
            _transactions = NULL;
        }
    } else {
        w = BitWalletNewByCoinType(NULL, 0, pubKey, BIP44_COIN_TYPE_BTC);
    }

    BitWalletSetCallbacks(w, NULL, balanceChanged, txAdded, txUpdated, txDeleted);
    _wallet = w;

    if (!_wallet) {
        __android_log_print(ANDROID_LOG_ERROR, "Message from C: ", "WARNING, _wallet is NULL!");
        return;
    }

    //create class
    jclass clazz = (*env)->FindClass(env, "com/bitwallet/blockchain/BitWalletManager");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "onBalanceChanged", "(J)V");
    //call java methods
    (*env)->CallStaticVoidMethod(env, clazz, mid, BitWalletBalance(_wallet));
}

JNIEXPORT jbyteArray
Java_com_jniwrappers_MnemonicCode_getMasterPubKey(JNIEnv *env, jobject thiz,
                                                  jbyteArray phrase, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getMasterPubKey");
    (*env)->GetArrayLength(env, phrase);
    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
    UInt512 key = UINT512_ZERO;
    char *charPhrase = (char *) bytePhrase;
    BitWBIP39DeriveKey(key.u8, charPhrase, NULL);
    BitWMasterPubKey pubKey = BitWBIP44MasterPubKey(key.u8, sizeof(key), coinType);

    size_t pubKeySize = sizeof(pubKey);
    jbyte *pubKeyBytes = (jbyte *) &pubKey;

    jbyteArray result = (*env)->NewByteArray(env, (jsize) pubKeySize);
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) pubKeySize, (const jbyte *) pubKeyBytes);
    (*env)->ReleaseByteArrayElements(env, phrase, bytePhrase, JNI_ABORT);
    //release everything
    return result;
}

//Call multiple times with all the transactions from the DB
JNIEXPORT void
Java_com_jniwrappers_BitWallet_putTransaction(JNIEnv *env, jobject thiz,
                                              jbyteArray transaction,
                                              jlong jBlockHeight,
                                              jlong jTimeStamp) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "putTransaction");
    if (!_transactions) return;

    int txLength = (*env)->GetArrayLength(env, transaction);
    jbyte *byteTx = (*env)->GetByteArrayElements(env, transaction, 0);

    assert(byteTx != NULL);
    if (!byteTx) return;

    BitWTransaction *tmpTx = BitWTransactionParseByCoinType((uint8_t *) byteTx, (size_t) txLength,
                                                            BIP44_COIN_TYPE_BTC);

    assert(tmpTx != NULL);
    if (!tmpTx) return;
    tmpTx->blockHeight = (uint32_t) jBlockHeight;
    tmpTx->timestamp = (uint32_t) jTimeStamp;
//    __android_log_print(ANDROID_LOG_ERROR, "Message from C: ", "tmpTx->timestamp: %u",
//                        tmpTx->timestamp);
//    __android_log_print(ANDROID_LOG_ERROR, "Message from C: ", "tmpTx: %s", u256_hex_encode(tmpTx->txHash));
    _transactions[_transactionsCounter++] = tmpTx;

}

JNIEXPORT void JNICALL
Java_com_jniwrappers_BitWallet_createTxArrayWithCount(JNIEnv *env, jobject thiz,
                                                      int txCount) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "createTxArrayWithCount: %d",
                        txCount);
    _transactions = calloc((size_t) txCount, sizeof(*_transactions));
    _transactionsCounter = 0;
    // need to call free(transactions);
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_getFirstReceiveAddress(JNIEnv *env,
                                                      jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getReceiveAddress");
    if (!_wallet) return NULL;

    BitWAddress receiveAddress = BitWalletReceiveAddress(_wallet);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "receiveAddress: %s",
                        receiveAddress.s);
    return (*env)->NewStringUTF(env, receiveAddress.s);
}

JNIEXPORT jobjectArray JNICALL Java_com_jniwrappers_BitWallet_getTransactions(
        JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getTransactions");
    if (!_wallet) return NULL;
    if (BitWalletTransactions(_wallet, NULL, 0) == 0) return NULL;
    //Retrieve the txs array

    size_t txCount = BitWalletTransactions(_wallet, NULL, 0);
    BitWTransaction **transactions_sqlite = calloc(BitWalletTransactions(_wallet, NULL, 0),
                                                   sizeof(BitWTransaction *));

    txCount = BitWalletTransactions(_wallet, transactions_sqlite, txCount);

    //Find the class and populate the array of objects of this class
    jclass txClass = (*env)->FindClass(env, "com/btbase/wallet/presenter/entities/TxItem");
    jobjectArray txObjects = (*env)->NewObjectArray(env, (jsize) txCount, txClass, 0);
    jobjectArray globalTxs = (*env)->NewGlobalRef(env, txObjects);
    jmethodID txObjMid = (*env)->GetMethodID(env, txClass, "<init>",
                                             "(JI[BLjava/lang/String;JJJ[Ljava/lang/String;[Ljava/lang/String;JI[JZ)V");
    jclass stringClass = (*env)->FindClass(env, "java/lang/String");

    for (int i = 0; i < txCount; i++) {
        if (!_wallet) return NULL;
        BitWTransaction *tempTx = transactions_sqlite[i];
        jboolean isValid = (jboolean) ((BitWalletTransactionIsValid(_wallet, tempTx) == 1)
                                       ? JNI_TRUE
                                       : JNI_FALSE);
        jlong JtimeStamp = tempTx->timestamp;
        jint JblockHeight = tempTx->blockHeight;
        jint JtxSize = (jint) BitWTransactionSize(tempTx);
        UInt256 txid = tempTx->txHash;
        jbyteArray JtxHash = (*env)->NewByteArray(env, sizeof(txid));
        (*env)->SetByteArrayRegion(env, JtxHash, 0, (jsize) sizeof(txid), (jbyte *) txid.u8);
        jlong Jsent = (jlong) BitWalletAmountSentByTx(_wallet, tempTx);
        jlong Jreceived = (jlong) BitWalletAmountReceivedFromTx(_wallet, tempTx);
        jlong Jfee = (jlong) BitWalletFeeForTx(_wallet, tempTx);
        int outCountTemp = (int) tempTx->outCount;
        jlongArray JoutAmounts = (*env)->NewLongArray(env, outCountTemp);
        jobjectArray JtoAddresses = (*env)->NewObjectArray(env, outCountTemp, stringClass, 0);

        int outCountAfterFilter = 0;

        for (int j = 0; j < outCountTemp; j++) {
            if (Jsent > 0) {
                if (!BitWalletContainsAddress(_wallet, tempTx->outputs[j].address)) {
                    jstring str = (*env)->NewStringUTF(env,
                                                       tempTx->outputs[j].address);
                    (*env)->SetObjectArrayElement(env, JtoAddresses, outCountAfterFilter, str);
                    (*env)->SetLongArrayRegion(env, JoutAmounts, outCountAfterFilter++, 1,
                                               (const jlong *) &tempTx->outputs[j].amount);
                    (*env)->DeleteLocalRef(env, str);
                }

            } else if (BitWalletContainsAddress(_wallet, tempTx->outputs[j].address)) {
                jstring str = (*env)->NewStringUTF(env, tempTx->outputs[j].address);
                (*env)->SetObjectArrayElement(env, JtoAddresses, outCountAfterFilter, str);
                (*env)->SetLongArrayRegion(env, JoutAmounts, outCountAfterFilter++, 1,
                                           (const jlong *) &tempTx->outputs[j].amount);
                (*env)->DeleteLocalRef(env, str);
            }
        }

        int inCountTemp = (int) transactions_sqlite[i]->inCount;
        jobjectArray JfromAddresses = (*env)->NewObjectArray(env, inCountTemp, stringClass, 0);

        int inCountAfterFilter = 0;

        for (int j = 0; j < inCountTemp; j++) {
            if (Jsent > 0) {
                jstring str = (*env)->NewStringUTF(env, tempTx->inputs[j].address);

                (*env)->SetObjectArrayElement(env, JfromAddresses, inCountAfterFilter++, str);
                (*env)->DeleteLocalRef(env, str);
            } else {
                jstring str = (*env)->NewStringUTF(env, tempTx->inputs[j].address);

                (*env)->SetObjectArrayElement(env, JfromAddresses, inCountAfterFilter++, str);
                (*env)->DeleteLocalRef(env, str);
            }
        }

        jlong JbalanceAfterTx = (jlong) BitWalletBalanceAfterTx(_wallet, tempTx);

        jobject txObject = (*env)->NewObject(env, txClass, txObjMid, JtimeStamp, JblockHeight,
                                             JtxHash, NULL, Jsent,
                                             Jreceived, Jfee, JtoAddresses, JfromAddresses,
                                             JbalanceAfterTx, JtxSize,
                                             JoutAmounts, isValid);

        (*env)->SetObjectArrayElement(env, globalTxs, (jsize) (txCount - 1 - i), txObject);
        (*env)->DeleteLocalRef(env, txObject);
        (*env)->DeleteLocalRef(env, JfromAddresses);
        (*env)->DeleteLocalRef(env, JtoAddresses);
        (*env)->DeleteLocalRef(env, JoutAmounts);
        (*env)->DeleteLocalRef(env, JtxHash);

    }

    if (transactions_sqlite) {
        free(transactions_sqlite);
        transactions_sqlite = NULL;
    }

    return globalTxs;
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_validateAddress(JNIEnv *env, jobject obj,
                                               jstring address, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "validateAddress");

    const char *str = (*env)->GetStringUTFChars(env, address, NULL);
//    int result = BitWAddressIsValid(str);
    int result = BitWAddressIsValidByCoinType(str, (int) coinType);

    (*env)->ReleaseStringUTFChars(env, address, str);
    return (jboolean) (result ? JNI_TRUE : JNI_FALSE);

}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_addressContainedInWallet(JNIEnv *env, jobject obj,
                                                        jstring address) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "addressContainedInWallet");
    if (!_wallet) return JNI_FALSE;

    const char *str = (*env)->GetStringUTFChars(env, address, NULL);
    int result = BitWalletContainsAddress(_wallet, str);

    (*env)->ReleaseStringUTFChars(env, address, str);
    return (jboolean) (result ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jlong JNICALL
Java_com_jniwrappers_BitWallet_getMinOutputAmount(JNIEnv *env, jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getMinOutputAmount");

    if (!_wallet) return 0;
    return (jlong) BitWalletMinOutputAmount(_wallet);
}

JNIEXPORT jlong JNICALL
Java_com_jniwrappers_BitWallet_getMinOutputAmountRequested(JNIEnv *env,
                                                           jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getMinOutputAmountRequested");
    return (jlong) TX_MIN_OUTPUT_AMOUNT;
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_addressIsUsed(JNIEnv *env, jobject obj,
                                             jstring address) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "addressIsUsed");
    if (!_wallet) return JNI_FALSE;

    const char *str = (*env)->GetStringUTFChars(env, address, NULL);
    int result = BitWalletAddressIsUsed(_wallet, str);

    (*env)->ReleaseStringUTFChars(env, address, str);
    return (jboolean) (result ? JNI_TRUE : JNI_FALSE);
}


JNIEXPORT jlong JNICALL
Java_com_jniwrappers_BitWallet_getMaxOutputAmount(JNIEnv *env, jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getMaxOutputAmount");
    assert(_wallet);
    if (!_wallet) return -1;
    return (jlong) BitWalletMaxOutputAmount(_wallet);
}

JNIEXPORT jint JNICALL
Java_com_jniwrappers_BitWallet_feeForTransaction(JNIEnv *env, jobject obj,
                                                 jstring address, jlong amount) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "feeForTransaction");
    if (!_wallet) return 0;
    const char *rawAddress = (*env)->GetStringUTFChars(env, address, NULL);
    BitWTransaction *tx = BitWalletCreateTransaction(_wallet, (uint64_t) amount, rawAddress);
    if (!tx) return 0;
    return (jint) BitWalletFeeForTx(_wallet, tx);
}

JNIEXPORT jlong JNICALL
Java_com_jniwrappers_BitWallet_feeForTransactionAmount(JNIEnv *env, jobject obj,
                                                       jlong amount) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "feeForTransaction");
    if (!_wallet) return 0;
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ",
                        "current fee: %lu", BitWalletFeePerKb(_wallet));
    uint64_t fee = BitWalletFeeForTxAmount(_wallet, (uint64_t) amount);
    return (jlong) fee;
}

JNIEXPORT jbyteArray JNICALL
Java_com_jniwrappers_BitWallet_tryTransaction(JNIEnv *env, jobject obj,
                                              jstring jAddress, jlong jAmount) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "tryTransaction");
    if (!_wallet) return 0;

    const char *rawAddress = (*env)->GetStringUTFChars(env, jAddress, NULL);
    BitWTransaction *tx = BitWalletCreateTransaction(_wallet, (uint64_t) jAmount, rawAddress);

    if (!tx) return NULL;

    size_t len = BitWTransactionSerialize(tx, NULL, 0);
    uint8_t *buf = malloc(len);

    len = BitWTransactionSerialize(tx, buf, len);

    jbyteArray result = (*env)->NewByteArray(env, (jsize) len);

    (*env)->SetByteArrayRegion(env, result, 0, (jsize) len, (jbyte *) buf);
    free(buf);
    return result;
}

JNIEXPORT jboolean JNICALL Java_com_jniwrappers_BitWallet_isCreated(JNIEnv *env,
                                                                    jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "wallet isCreated %s",
                        _wallet ? "yes" : "no");
    return (jboolean) (_wallet ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_transactionIsVerified(JNIEnv *env, jobject obj,
                                                     jstring jtxHash) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "transactionIsVerified");
    if (!_wallet) return JNI_FALSE;

    const char *txHash = (*env)->GetStringUTFChars(env, jtxHash, NULL);
    UInt256 txHashResult = u256_hex_decode(txHash);
    BitWTransaction *tx = BitWalletTransactionForHash(_wallet, txHashResult);

    if (!tx) return JNI_FALSE;

    int result = BitWalletTransactionIsVerified(_wallet, tx);

    return (jboolean) (result ? JNI_TRUE : JNI_FALSE);
}


JNIEXPORT jlong JNICALL
Java_com_jniwrappers_BitWallet_bitcoinAmount(JNIEnv *env, jobject thiz,
                                             jlong localAmount, double price) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ",
                        "bitcoinAmount: localAmount: %lli, price: %lf", localAmount, price);
    return (jlong) BitWBitcoinAmount(localAmount, price);
}

JNIEXPORT jlong
Java_com_jniwrappers_BitWallet_localAmount(JNIEnv *env, jobject thiz, jlong amount,
                                           double price) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ",
                        "localAmount: amount: %lli, price: %lf", amount, price);
    return (jlong) BitWLocalAmount(amount, price);
}

JNIEXPORT void JNICALL
Java_com_jniwrappers_BitWallet_walletFreeEverything(JNIEnv *env, jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "walletFreeEverything");

    if (_wallet) {
        BitWalletFree(_wallet, BIP44_COIN_TYPE_BTC);
        _wallet = NULL;
    }

    if (_transactions) {
        _transactions = NULL;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_validateRecoveryPhrase(JNIEnv *env, jobject obj,
                                                      jobjectArray stringArray,
                                                      jstring jPhrase) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "validateRecoveryPhrase");

    int wordsCount = (*env)->GetArrayLength(env, stringArray);
    char *wordList[wordsCount];

    for (int i = 0; i < wordsCount; i++) {
        jstring string = (jstring) (*env)->GetObjectArrayElement(env, stringArray, i);
        const char *rawString = (*env)->GetStringUTFChars(env, string, 0);

        wordList[i] = malloc(strlen(rawString) + 1);
        strcpy(wordList[i], rawString);
        (*env)->ReleaseStringUTFChars(env, string, rawString);
        (*env)->DeleteLocalRef(env, string);
    }

    const char *str = (*env)->GetStringUTFChars(env, jPhrase, NULL);
    int result = BitWBIP39PhraseIsValid((const char **) wordList, str);

    (*env)->ReleaseStringUTFChars(env, jPhrase, str);

    return (jboolean) (result ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_getAddress(JNIEnv *env, jobject thiz,
                                          jbyteArray bytePubKey, jint coinType, jint change,
                                          jint index) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getFirstAddress");

//    BitWAddress address = BitW_ADDRESS_NONE;
    char address[256];
    memset(address, 0, sizeof(address));
    jbyte *pubKeyBytes = (*env)->GetByteArrayElements(env, bytePubKey, 0);
    BitWMasterPubKey mpk = *(BitWMasterPubKey *) pubKeyBytes;
    uint8_t pubKey[33];
    BitWKey key;

    BitWBIP44PubKey(pubKey, sizeof(pubKey), mpk, change, index);
    BitWKeySetPubKey(&key, pubKey, sizeof(pubKey));

    char *coinpubkey = uint8toHex(pubKey, sizeof(pubKey));
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ",
                        "cointype:%d\npubkey:%s\n",
                        coinType, coinpubkey);

    BitWKeyAddressByCoinType(&key, address, sizeof(address), coinType);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "address:%s\n", address);
    return (*env)->NewStringUTF(env, address);
}

JNIEXPORT jbyteArray JNICALL
Java_com_jniwrappers_BitWallet_publishSerializedTransaction(JNIEnv *env,
                                                            jobject thiz,
                                                            jbyteArray serializedTransaction,
                                                            jbyteArray phrase) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "publishSerializedTransaction");
//    if (!_peerManager) return NULL;

    int txLength = (*env)->GetArrayLength(env, serializedTransaction);
    jbyte *byteTx = (*env)->GetByteArrayElements(env, serializedTransaction, 0);
    BitWTransaction *tmpTx = BitWTransactionParseByCoinType((uint8_t *) byteTx, (size_t) txLength,
                                                            BIP44_COIN_TYPE_BTC);

    if (!tmpTx) return NULL;

    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
    UInt512 key = UINT512_ZERO;
    char *charPhrase = (char *) bytePhrase;
    BitWBIP39DeriveKey(key.u8, charPhrase, NULL);

    size_t seedSize = sizeof(key);

    BitWalletSignTransactionByCoinType(_wallet, tmpTx, 0, key.u8, seedSize, BIP44_COIN_TYPE_BTC);
    assert(BitWTransactionIsSigned(tmpTx));
    if (!tmpTx) return NULL;
//    BitWPeerManagerPublishTx(_peerManager, tmpTx, tmpTx, callback);
    (*env)->ReleaseByteArrayElements(env, phrase, bytePhrase, JNI_ABORT);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "returning true");
    UInt256 txid = tmpTx->txHash;
    jbyteArray JtxHash = (*env)->NewByteArray(env, sizeof(txid));
    (*env)->SetByteArrayRegion(env, JtxHash, 0, (jsize) sizeof(txid), (jbyte *) txid.u8);

    return JtxHash;
}


JNIEXPORT jbyteArray JNICALL
Java_com_jniwrappers_BitWallet_signTransaction(JNIEnv *env, jobject thiz,
                                               jint coinType,
                                               jbyteArray serializedTransaction,
                                               jbyteArray phrase,
                                               jstring toAddress,
                                               jobjectArray addressArray,
                                               jlongArray amountArray) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "publishSerializedTransaction");

    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
    UInt512 key = UINT512_ZERO;
    char *charPhrase = (char *) bytePhrase;
    BitWBIP39DeriveKey(key.u8, charPhrase, NULL);
    size_t seedSize = sizeof(key);

    BitWMasterPubKey pubKey = BitWBIP44MasterPubKey(key.u8, sizeof(key), coinType);
    BitWallet *w = BitWalletNewByCoinType(NULL, 0, pubKey, coinType);//分配内存
    //----------------------------------------------------------------------------------------------
    //解析address
    int count = (*env)->GetArrayLength(env, addressArray);
    const char *addressList[count];

    for (int i = 0; i < count; i++) {
        jstring string = (jstring) (*env)->GetObjectArrayElement(env, addressArray, i);
        const char *rawString = (*env)->GetStringUTFChars(env, string, 0);
        addressList[i] = rawString;
        (*env)->DeleteLocalRef(env, string);
    }
    //解析amount
    count = (*env)->GetArrayLength(env, amountArray);
    jlong *amoutList = NULL;
    amoutList = (*env)->GetLongArrayElements(env, amountArray, NULL);
    //----------------------------------------------------------------------------------------------

    int txLength = (*env)->GetArrayLength(env, serializedTransaction);
    jbyte *byteTx = (*env)->GetByteArrayElements(env, serializedTransaction, 0);
    BitWTransaction *tmpTx = BitWTransactionParseByCoinType((uint8_t *) byteTx, (size_t) txLength,
                                                            coinType);

    if (!tmpTx) return NULL;

    //----------------------------------------------------------------------------------------------
    BitWTxInput *input;
    int inCount = tmpTx->inCount;

    for (int i = 0; i < inCount && i < count; i++) {
        input = &tmpTx->inputs[i];
        input->amount = amoutList[i];
        BitWTxInputSetAddressByCoinType(input, addressList[i], coinType);

        __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "SetScript:%s",
                            uint8toHex(input->script, input->scriptLen));
    }
    //----------------------------------------------------------------------------------------------
    int outCount = tmpTx->outCount;
    BitWTxOutput *output;
    const char *outAddress = (*env)->GetStringUTFChars(env, toAddress, NULL);

    int outAddressCount = 0;
    for (int i = 0; i < outCount; i++) {
        output = &tmpTx->outputs[i];
        if (BitWAddressEq(output->address, outAddress)) {
            outAddressCount++;
            __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "toAddress:%s",
                                output->address);
        }
    }

    //----------------------------------------------------------------------------------------------

    if (outAddressCount > 0) {//outaddressCout <=0 说明 地址被串改 不走签名逻辑
        switch (coinType) {
            case BIP44_COIN_TYPE_BCH: {
                BitWalletSignTransactionByCoinType(w, coinType, tmpTx, 0x40, key.u8, seedSize);
                break;
            }
            case BIP44_COIN_TYPE_BTC:
            case BIP44_COIN_TYPE_QTUM:
            case BIP44_COIN_TYPE_BTN:
            default: {
                BitWalletSignTransactionByCoinType(w, coinType, tmpTx, 0, key.u8, seedSize);
                break;
            }
        }
    }

    BitWalletFree(w, coinType);//释放wallet

    assert(BitWTransactionIsSigned(tmpTx));
    if (!tmpTx) return NULL;

    uint8_t buf[BitWTransactionSerialize(tmpTx, NULL, 0)]; // serializing/parsing unsigned tx
    size_t txLen = BitWTransactionSerialize(tmpTx, buf, sizeof(buf));

    jbyteArray serializedTx = (*env)->NewByteArray(env, (jsize) txLen);
    (*env)->SetByteArrayRegion(env, serializedTx, 0, (jsize) txLen, (jbyte *) buf);

    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "returning true");

//    jstring result = (*env)->NewStringUTF(env, buf);
    return serializedTx;
}

JNIEXPORT jlong JNICALL Java_com_jniwrappers_BitWallet_getTotalSent(JNIEnv *env,
                                                                    jobject obj) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getTotalSent");
    if (!_wallet) return 0;
    return (jlong) BitWalletTotalSent(_wallet);
}

JNIEXPORT void JNICALL Java_com_jniwrappers_BitWallet_setFeePerKb(JNIEnv *env,
                                                                  jobject obj,
                                                                  jlong fee,
                                                                  jboolean ignore) {
    if (!_wallet || ignore) return;
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "setFeePerKb, ignore:%d, fee: %lli",
                        ignore, fee);
    BitWalletSetFeePerKb(_wallet, (uint64_t) fee);
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_isValidBitcoinPrivateKey(JNIEnv *env,
                                                        jobject instance,
                                                        jstring key, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "isValidBitcoinPrivateKey");

    const char *privKey = (*env)->GetStringUTFChars(env, key, NULL);
    int result = BitWPrivKeyIsValid(privKey, coinType);

    (*env)->ReleaseStringUTFChars(env, key, privKey);
    return (jboolean) ((result == 1) ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_isValidBitcoinBIP38Key(JNIEnv *env, jobject instance,
                                                      jstring key) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "isValidBitcoinBIP38Key");

    const char *privKey = (*env)->GetStringUTFChars(env, key, NULL);
    int result = BitWBIP38KeyIsValid(privKey);

    (*env)->ReleaseStringUTFChars(env, key, privKey);
    return (jboolean) ((result == 1) ? JNI_TRUE : JNI_FALSE);
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_getAddressFromPrivKey(JNIEnv *env, jobject instance,
                                                     jstring privKey, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getAddressFromPrivKey");

    const char *rawPrivKey = (*env)->GetStringUTFChars(env, privKey, NULL);
    BitWKey key;
    BitWAddress addr;

    BitWKeySetPrivKey(&key, rawPrivKey, coinType);
    BitWKeyAddressByCoinType(&key, addr.s, sizeof(addr), BIP44_COIN_TYPE_BTC);

    jstring result = (*env)->NewStringUTF(env, addr.s);
    return result;
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_decryptBip38Key(JNIEnv *env, jobject instance,
                                               jstring privKey,
                                               jstring pass, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "decryptBip38Key");

    BitWKey key;
    const char *rawPrivKey = (*env)->GetStringUTFChars(env, privKey, NULL);
    const char *rawPass = (*env)->GetStringUTFChars(env, pass, NULL);
    int result = BitWKeySetBIP38Key(&key, rawPrivKey, rawPass);

    if (result) {
        char pk[BitWKeyPrivKey(&key, NULL, 0, coinType)];

        BitWKeyPrivKey(&key, pk, sizeof(pk), coinType);
        return (*env)->NewStringUTF(env, pk);
    } else return (*env)->NewStringUTF(env, "");
}

JNIEXPORT void JNICALL Java_com_jniwrappers_BitWallet_createInputArray(JNIEnv *env,
                                                                       jobject thiz) {
    if (_privKeyTx) {
        BitWTransactionFree(_privKeyTx, BIP44_COIN_TYPE_BTC);
        _privKeyTx = NULL;
    }

    _privKeyBalance = 0;
    _privKeyTx = BitWTransactionNew();
}

JNIEXPORT void JNICALL
Java_com_jniwrappers_BitWallet_addInputToPrivKeyTx(JNIEnv *env, jobject thiz,
                                                   jbyteArray hash, int vout,
                                                   jbyteArray script,
                                                   jlong amount) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "addInputToPrivKeyTx");
    _privKeyBalance += amount;

    jsize hashLength = (*env)->GetArrayLength(env, hash);
    jsize scriptLength = (*env)->GetArrayLength(env, script);

    if (hashLength > 256 || !_privKeyTx) return;

    jbyte *rawHash = (*env)->GetByteArrayElements(env, hash, 0);
    jbyte *rawScript = (*env)->GetByteArrayElements(env, script, 0);
    UInt256 reversedHash = UInt256Reverse((*(UInt256 *) rawHash));

    BitWTransactionAddInput(_privKeyTx, reversedHash, (uint32_t) vout, (uint64_t) amount,
                            (const uint8_t *) rawScript,
                            (size_t) scriptLength, NULL, 0, TXIN_SEQUENCE, BIP44_COIN_TYPE_BTC);
}

JNIEXPORT jobject JNICALL
Java_com_jniwrappers_BitWallet_getPrivKeyObject(JNIEnv *env,
                                                jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getPrivKeyObject");
    if (!_privKeyTx) return NULL;

    jclass importPrivKeyClass = (*env)->FindClass(env,
                                                  "com/btbase/wallet/presenter/entities/ImportPrivKeyEntity");
    BitWAddress address = BitWalletReceiveAddress(_wallet);
    uint8_t script[BitWAddressScriptPubKeyByCoinType(NULL, 0, address.s, BIP44_COIN_TYPE_BTC)];
    size_t scriptLen = BitWAddressScriptPubKeyByCoinType(script, sizeof(script), address.s,
                                                         BIP44_COIN_TYPE_BTC);

    BitWTransactionAddOutput(_privKeyTx, 0, script, scriptLen, BIP44_COIN_TYPE_BTC);

    uint64_t fee = BitWalletFeeForTxSize(_wallet, BitWTransactionSize(_privKeyTx));

    _privKeyTx->outputs[0].amount = _privKeyBalance - fee;

    jmethodID txObjMid = (*env)->GetMethodID(env, importPrivKeyClass, "<init>", "([BJJ)V");

    uint8_t buf[BitWTransactionSerialize(_privKeyTx, NULL, 0)];
    size_t len = BitWTransactionSerialize(_privKeyTx, buf, sizeof(buf));
    jbyteArray result = (*env)->NewByteArray(env, (jsize) len);

    (*env)->SetByteArrayRegion(env, result, 0, (jsize) len, (jbyte *) buf);

    jobject txObject = (*env)->NewObject(env, importPrivKeyClass, txObjMid, result,
                                         _privKeyBalance - fee, fee);
    return txObject;
}

JNIEXPORT jboolean JNICALL
Java_com_jniwrappers_BitWallet_confirmKeySweep(JNIEnv *env, jobject thiz,
                                               jbyteArray tx, jstring privKey) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "confirmKeySweep");
//    assert(_peerManager);
//    if (!_peerManager) return JNI_FALSE;

    int txLength = (*env)->GetArrayLength(env, tx);
    jbyte *byteTx = (*env)->GetByteArrayElements(env, tx, 0);
    BitWTransaction *tmpTx = BitWTransactionParseByCoinType((uint8_t *) byteTx, (size_t) txLength,
                                                            BIP44_COIN_TYPE_BTC);
    assert(tmpTx);
    if (!tmpTx) return JNI_FALSE;

    const char *rawString = (*env)->GetStringUTFChars(env, privKey, 0);
    BitWKey key;

    BitWKeySetPrivKey(&key, rawString, BIP44_COIN_TYPE_BTC);
    BitWTransactionSignByCoinType(tmpTx, 0, &key, 1, BIP44_COIN_TYPE_BTC);
    if (!tmpTx || !BitWTransactionIsSigned(tmpTx)) return JNI_FALSE;

    uint8_t buf[BitWTransactionSerialize(tmpTx, NULL, 0)];
    size_t len = BitWTransactionSerialize(tmpTx, buf, sizeof(buf));

//    BitWPeerManagerPublishTx(_peerManager, tmpTx, NULL, callback);
    return JNI_TRUE;
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_reverseTxHash(JNIEnv *env, jobject thiz,
                                             jstring txHash) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "reverseTxHash");

    const char *rawString = (*env)->GetStringUTFChars(env, txHash, 0);
    UInt256 theHash = u256_hex_decode(rawString);
    UInt256 reversedHash = UInt256Reverse(theHash);

    return (*env)->NewStringUTF(env, u256_hex_encode(reversedHash));
}

JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_txHashToHex(JNIEnv *env, jobject thiz,
                                           jbyteArray txHash) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "txHashToHex");

//    int hashLen = (*env)->GetArrayLength(env, txHash);
    jbyte *hash = (*env)->GetByteArrayElements(env, txHash, 0);

    UInt256 reversedHash = UInt256Reverse((*(UInt256 *) hash));

//    const char *rawString = (*env)->GetStringUTFChars(env, txHash, 0);
//    UInt256 theHash = u256_hex_decode(rawString);
//    UInt256 reversedHash = UInt256Reverse(theHash);
//
    return (*env)->NewStringUTF(env, u256_hex_encode(reversedHash));
}


JNIEXPORT jstring JNICALL
Java_com_jniwrappers_BitWallet_txHashSha256Hex(JNIEnv *env, jobject thiz,
                                               jstring txHash) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "reverseTxHash");

    const char *rawString = (*env)->GetStringUTFChars(env, txHash, 0);
    UInt256 theHash = u256_hex_decode(rawString);
//    UInt256 reversedHash = UInt256Reverse(theHash);
//    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "reversedHash: %s", u256_hex_encode(reversedHash));
    UInt256 sha256Hash;
    BitWSHA256(&sha256Hash, theHash.u8, sizeof(theHash));

//    UInt256 reversedHash = UInt256Reverse(sha256Hash);
    char *result = u256_hex_encode(sha256Hash);
    return (*env)->NewStringUTF(env, result);
}

JNIEXPORT jint JNICALL Java_com_jniwrappers_BitWallet_getTxCount(JNIEnv *env,
                                                                 jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getTxCoun");
    if (!_wallet) return 0;
    return (jint) BitWalletTransactions(_wallet, NULL, 0);
}

/**
 * 解压私钥
 * @param env
 * @param thiz
 * @param seed
 * @return
 */
JNIEXPORT jstring JNICALL Java_com_jniwrappers_BitWallet_uncompressPrivKey(
        JNIEnv *env,
        jobject thiz,
        jbyteArray privkey, jint coinType) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "uncompressPrivKey");

    jbyte *bytePrivKey = (*env)->GetByteArrayElements(env, privkey, 0);
    BitWKey key;
    BitWKeySetPrivKey(&key, (const char *) bytePrivKey, coinType);
// uncompressed private key export
    key.compressed = 0;
    char privKey1[BitWKeyPrivKey(&key, NULL, 0, coinType)];
    BitWKeyPrivKey(&key, privKey1, sizeof(privKey1), coinType);
#if BITCOIN_TESTNET
    char *secKey = u256_hex_encode(key.secret);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C:", "uncompressPrivKey eos secret: %s", secKey);
#endif
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C:", "uncompressPrivKey: %s", privKey1);

    jstring result = (*env)->NewStringUTF(env, privKey1);
    return result;
}

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_MnemonicCode_getPrivKeyByCoinType(
        JNIEnv *env,
        jobject thiz,
        jbyteArray seed,
        jint coinType, jint change, jint index) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getPrivKeyByCoinType");
    jbyte *bytesSeed = (*env)->GetByteArrayElements(env, seed, 0);
    size_t seedLen = (size_t) (*env)->GetArrayLength(env, seed);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "cointype:%d, seedLen: %d",coinType, (int) seedLen);
    BitWKey key;
    BitWBIP44PrivKey(&key, bytesSeed, seedLen, (int) coinType, change, index);

    jbyteArray result;

    switch (coinType) {
        case BIP44_COIN_TYPE_ETH: {
            char *privkey = u256_hex_encode(key.secret);
            result = (*env)->NewByteArray(env, (jsize) strlen(privkey));
            (*env)->SetByteArrayRegion(env, result, 0, (jsize) strlen(privkey), (jbyte *) privkey);
            __android_log_print(ANDROID_LOG_DEBUG, "Message from C:", "eth secret: %s", privkey);
            break;
        }
        case BIP44_COIN_TYPE_EOS:{
#if BITCOIN_TESTNET
            char *secKey = u256_hex_encode(key.secret);
            __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "eos secret: %s", secKey);
#endif
            key.compressed = 0;
            char rawKey[BitWKeyPrivKey(&key, NULL, 0, coinType)];
            BitWKeyPrivKey(&key, rawKey, sizeof(rawKey),coinType);
            __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "rawKey: %s", rawKey);
            result = (*env)->NewByteArray(env, (jsize) sizeof(rawKey));
            (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(rawKey), (jbyte *) rawKey);
            break;
        }
        default: {

            char rawKey[BitWKeyPrivKey(&key, NULL, 0,coinType)];
            BitWKeyPrivKey(&key, rawKey, sizeof(rawKey),coinType);
            __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "rawKey: %s", rawKey);

            result = (*env)->NewByteArray(env, (jsize) sizeof(rawKey));
            (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(rawKey), (jbyte *) rawKey);
            break;
        }
    }
    return result;
}


JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_MnemonicCode_getAuthPrivKeyForAPI(
        JNIEnv *env,
        jobject thiz,
        jbyteArray seed) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getAuthPrivKeyForAPI");
    jbyte *bytesSeed = (*env)->GetByteArrayElements(env, seed, 0);
    size_t seedLen = (size_t) (*env)->GetArrayLength(env, seed);
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "seedLen: %d", (int) seedLen);
    BitWKey key;
    BitWBIP32APIAuthKey(&key, bytesSeed, seedLen);
    char rawKey[BitWKeyPrivKey(&key, NULL, 0, BIP44_COIN_TYPE_BTC)];
    BitWKeyPrivKey(&key, rawKey, sizeof(rawKey), BIP44_COIN_TYPE_BTC);
//    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "rawKey: %s", rawKey);
    jbyteArray result = (*env)->NewByteArray(env, (jsize) sizeof(rawKey));
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(rawKey), (jbyte *) rawKey);
    return result;
}

JNIEXPORT jstring JNICALL Java_com_jniwrappers_BitWallet_getAuthPublicKeyForAPI(
        JNIEnv *env,
        jobject thiz,
        jbyteArray privkey) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getAuthPublicKeyForAPI");
    jbyte *bytePrivKey = (*env)->GetByteArrayElements(env, privkey, 0);
    BitWKey key;
    BitWKeySetPrivKey(&key, (const char *) bytePrivKey, BIP44_COIN_TYPE_BTC);

    size_t len = BitWKeyPubKey(&key, NULL, 0);
    uint8_t pubKey[len];
    BitWKeyPubKey(&key, &pubKey, len);
    size_t strLen = BitWBase58Encode(NULL, 0, pubKey, len);
    char base58string[strLen];
    BitWBase58Encode(base58string, strLen, pubKey, len);

    return (*env)->NewStringUTF(env, base58string);
}

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_MnemonicCode_getSeedFromPhrase(
        JNIEnv *env,
        jobject thiz,
        jbyteArray phrase) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getSeedFromPhrase");
    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);
    UInt512 key = UINT512_ZERO;
    char *charPhrase = (char *) bytePhrase;
    BitWBIP39DeriveKey(key.u8, charPhrase, NULL);
    jbyteArray result = (*env)->NewByteArray(env, (jsize) sizeof(key));
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(key), (jbyte *) &key);
    return result;
}

JNIEXPORT jboolean JNICALL Java_com_jniwrappers_BitWallet_isTestNet(JNIEnv *env,
                                                                    jobject thiz) {
    return BITCOIN_TESTNET ? JNI_TRUE : JNI_FALSE;
}

// returns an unsigned transaction that sweeps all wallet UTXOs as of block height 478559 to addr
// transaction must be signed using a forkId of 0x40
static BitWTransaction *
BitWalletBCashSweepTx(BitWallet *wallet, BitWMasterPubKey mpk, const char *addr,
                      uint64_t feePerKb) {
    size_t txCount = BitWalletTransactions(wallet, NULL, 0) -
                     BitWalletTxUnconfirmedBefore(wallet, NULL, 0, BCASH_FORKHEIGHT);
    BitWTransaction *transactions[txCount], *tx;
    BitWallet *w;

    txCount = BitWalletTransactions(wallet, transactions, txCount);
    w = BitWalletNewByCoinType(transactions, txCount, mpk, BIP44_COIN_TYPE_BTC);
    BitWalletSetFeePerKb(w, feePerKb);
    tx = BitWalletCreateTransaction(w, BitWalletMaxOutputAmount(w), addr);
    BitWalletFree(w, BIP44_COIN_TYPE_BTC);
    return tx;
}

JNIEXPORT jlong JNICALL Java_com_jniwrappers_BitWallet_getBCashBalance(
        JNIEnv *env,
        jobject thiz,
        jbyteArray bytePubKey) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getSeedFromPhrase");

    jbyte *pubKeyBytes = (*env)->GetByteArrayElements(env, bytePubKey, 0);
    BitWMasterPubKey pubKey = *(BitWMasterPubKey *) pubKeyBytes;

    size_t txCount = BitWalletTransactions(_wallet, NULL, 0) -
                     BitWalletTxUnconfirmedBefore(_wallet, NULL, 0, BCASH_FORKHEIGHT);
    BitWTransaction *transactions[txCount];
    BitWallet *w;

    txCount = BitWalletTransactions(_wallet, transactions, txCount);
    w = BitWalletNewByCoinType(transactions, txCount, pubKey, BIP44_COIN_TYPE_BTC);
    jlong balance = (jlong) BitWalletBalance(w);
//    BitWalletFree(w);
//
    return balance;
}

JNIEXPORT jint JNICALL Java_com_jniwrappers_BitWallet_getTxSize(
        JNIEnv *env,
        jobject thiz,
        jbyteArray serializedTransaction) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getSeedFromPhrase");

    int txLength = (*env)->GetArrayLength(env, serializedTransaction);
    jbyte *byteTx = (*env)->GetByteArrayElements(env, serializedTransaction, 0);
    BitWTransaction *tmpTx = BitWTransactionParseByCoinType((uint8_t *) byteTx, (size_t) txLength,
                                                            BIP44_COIN_TYPE_BTC);

    return (jint) (jlong) BitWTransactionSize(tmpTx);
}

JNIEXPORT jlong JNICALL Java_com_jniwrappers_BitWallet_nativeBalance(
        JNIEnv *env,
        jobject thiz) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getSeedFromPhrase");

//    int txLength = (*env)->GetArrayLength(env, serializedTransaction);
//    jbyte *byteTx = (*env)->GetByteArrayElements(env, serializedTransaction, 0);
//    BitWTransaction *tmpTx = BitWTransactionParse((uint8_t *) byteTx, (size_t) txLength);
    if (!_wallet) return -1;
    return BitWalletBalance(_wallet);
}

//creates and signs a bcash tx, returns the serialized tx
JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWallet_sweepBCash(JNIEnv *env,
                                                                       jobject thiz,
                                                                       jbyteArray bytePubKey,
                                                                       jstring address,
                                                                       jbyteArray phrase) {
    __android_log_print(ANDROID_LOG_DEBUG, "Message from C: ", "getSeedFromPhrase");

    if (!_wallet) return NULL;

    jbyte *pubKeyBytes = (*env)->GetByteArrayElements(env, bytePubKey, 0);
    BitWMasterPubKey pubKey = *(BitWMasterPubKey *) pubKeyBytes;
    const char *rawAddress = (*env)->GetStringUTFChars(env, address, NULL);
    jbyte *bytePhrase = (*env)->GetByteArrayElements(env, phrase, 0);

    UInt512 key = UINT512_ZERO;
    char *charPhrase = (char *) bytePhrase;
    BitWBIP39DeriveKey(key.u8, charPhrase, NULL);

    size_t seedSize = sizeof(key);

    BitWTransaction *tx = BitWalletBCashSweepTx(_wallet, pubKey, rawAddress, MIN_FEE_PER_KB);

    BitWalletSignTransactionByCoinType(_wallet, tx, 0x40, key.u8, seedSize, BIP44_COIN_TYPE_BTC);
    assert(BitWTransactionIsSigned(tx));
    if (!tx) return NULL;

    size_t len = BitWTransactionSerialize(tx, NULL, 0);
    uint8_t *buf = malloc(len);

    len = BitWTransactionSerialize(tx, buf, len);

    jbyteArray result = (*env)->NewByteArray(env, (jsize) len);

    (*env)->SetByteArrayRegion(env, result, 0, (jsize) len, (jbyte *) buf);
    free(buf);
    return result;

}

                 