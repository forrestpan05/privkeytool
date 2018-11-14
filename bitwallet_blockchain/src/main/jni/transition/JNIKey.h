//
// Created by Mihail Gutan on 10/9/16.
//

#ifndef BitWEADWALLET_JNIKEY_H
#define BitWEADWALLET_JNIKEY_H

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWKey_compactSign(JNIEnv *env, jobject thiz,
                                                                    jbyteArray data);

JNIEXPORT jboolean JNICALL Java_com_jniwrappers_BitWKey_setPrivKey(JNIEnv *env,
                                                                 jobject thiz,
                                                                 jbyteArray privKey, jint coinType);

JNIEXPORT void JNICALL Java_com_jniwrappers_BitWKey_setSecret(JNIEnv *env,
                                                            jobject thiz,
                                                            jbyteArray privKey);

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWKey_encryptNative(JNIEnv *env, jobject thiz,
                                                                      jbyteArray data,
                                                                      jbyteArray nonce);

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWKey_decryptNative(JNIEnv *env, jobject thiz,
                                                                      jbyteArray data,
                                                                      jbyteArray nonce);

JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWKey_address(JNIEnv *env, jobject thiz);

#ifdef __cplusplus
}
#endif

#endif //BitWEADWALLET_JNIKEY_H
