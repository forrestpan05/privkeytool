//
// Created by Mihail Gutan on 1/24/17.
//

#ifndef BitWEADWALLET_JNIBIP32SEQUENCE_H
#define BitWEADWALLET_JNIBIP32SEQUENCE_H

#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT jbyteArray JNICALL Java_com_jniwrappers_BitWBIP32Sequence_bip32BitIDKey(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jbyteArray seed,
                                                                             jint index,
                                                                             jstring strUri);


#ifdef __cplusplus
}
#endif

#endif //BitWEADWALLET_JNIBIP32SEQUENCE_H
