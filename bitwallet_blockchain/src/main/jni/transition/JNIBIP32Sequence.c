//
// Created by Mihail Gutan on 1/24/17.
//

#include <jni.h>
#include <BitWKey.h>
#include <BitWBIP32Sequence.h>
#include <BitWPeer.h>
#include "JNIBIP32Sequence.h"

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWBIP32Sequence_bip32BitIDKey(JNIEnv *env, jobject thiz,
                                                                      jbyteArray seed, jint index, jstring strUri) {
    int seedLength = (*env)->GetArrayLength(env, seed);
    const char *uri = (*env)->GetStringUTFChars(env, strUri, NULL);
    jbyte *byteSeed = (*env)->GetByteArrayElements(env, seed, 0);
    BitWKey key;

    BitWBIP32BitIDKey(&key, byteSeed, (size_t) seedLength, (uint32_t) index, uri);

    char rawKey[BitWKeyPrivKey(&key, NULL, 0, BIP44_COIN_TYPE_BTC)];
    BitWKeyPrivKey(&key, rawKey, sizeof(rawKey), BIP44_COIN_TYPE_BTC);
    jbyteArray result = (*env)->NewByteArray(env, (jsize) sizeof(rawKey));
    (*env)->SetByteArrayRegion(env, result, 0, (jsize) sizeof(rawKey), (jbyte *) rawKey);

    return result;
}