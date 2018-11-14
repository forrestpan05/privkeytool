# Sets the minimum version of CMake required to build your native library.
# This ensures that a certain set of CMake features is available to
# your build.

cmake_minimum_required(VERSION 3.4.1)

# Specifies a library name, specifies whether the library is STATIC or
# SHARED, and provides relative paths to the source code. You can
# define multiple libraries by adding multiple add_library() commands,
# and CMake builds them for you. When you build your app, Gradle
# automatically packages shared libraries with your APK.

# now build app's shared lib
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID_STL=gnustl_static -DANDROID_TOOLCHAIN=clang")

# -Wimplicit-function-declaration
# -Wno-missing-prototypes -Werror=return-type -Wdocumentation -Wunreachable-code-aggressive -Wno-missing-braces
# -Wparentheses -Wswitch -Wno-unused-function -Wunused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body
# -Wconditional-uninitialized -Wno-unknown-pragmas -pedantic -Wshadow -Wfour-char-constants -Wno-conversion -Wconstant-conversion
# -Wint-conversion -Wbool-conversion -Wenum-conversion -Wassign-enum -Wno-shorten-64-to-32 -Wpointer-sign -Wnewline-eof
# -Wdeprecated-declarations -Wno-sign-conversion

add_library( # Specifies the name of the library.
             bitw-core-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/jni/bitwallet-core/BitWAddress.c
             src/main/jni/bitwallet-core/BitWAddress.h
             src/main/jni/bitwallet-core/BitWArray.h
             src/main/jni/bitwallet-core/BitWBase58.c
             src/main/jni/bitwallet-core/BitWBase58.h
             src/main/jni/bitwallet-core/BitWBIP32Sequence.c
             src/main/jni/bitwallet-core/BitWBIP32Sequence.h
             src/main/jni/bitwallet-core/BitWBIP38Key.c
             src/main/jni/bitwallet-core/BitWBIP38Key.h
             src/main/jni/bitwallet-core/BitWBIP39Mnemonic.c
             src/main/jni/bitwallet-core/BitWBIP39Mnemonic.h
             src/main/jni/bitwallet-core/BitWBIP39WordsEn.h
             src/main/jni/bitwallet-core/BitWBloomFilter.c
             src/main/jni/bitwallet-core/BitWBloomFilter.h
             src/main/jni/bitwallet-core/BitWCrypto.c
             src/main/jni/bitwallet-core/BitWCrypto.h
             src/main/jni/bitwallet-core/BitWInt.h
             src/main/jni/bitwallet-core/BitWKey.c
             src/main/jni/bitwallet-core/BitWKey.h
             src/main/jni/bitwallet-core/BitWMerkleBlock.c
             src/main/jni/bitwallet-core/BitWMerkleBlock.h
             src/main/jni/bitwallet-core/BitWPaymentProtocol.c
             src/main/jni/bitwallet-core/BitWPaymentProtocol.h
             src/main/jni/bitwallet-core/BitWPeer.c
             src/main/jni/bitwallet-core/BitWPeer.h
             src/main/jni/bitwallet-core/BitWPeerManager.c
             src/main/jni/bitwallet-core/BitWPeerManager.h
             src/main/jni/bitwallet-core/BitWSet.c
             src/main/jni/bitwallet-core/BitWSet.h
             src/main/jni/bitwallet-core/BitWTransaction.c
             src/main/jni/bitwallet-core/BitWTransaction.h
             src/main/jni/bitwallet-core/BitWallet.c
             src/main/jni/bitwallet-core/BitWallet.h

             src/main/jni/transition/core.c
             src/main/jni/transition/core.h
             src/main/jni/transition/JNIBase58.c
             src/main/jni/transition/JNIBase58.h
             src/main/jni/transition/JNIBIP32Sequence.c
             src/main/jni/transition/JNIBIP32Sequence.h
             src/main/jni/transition/JNIKey.c
             src/main/jni/transition/JNIKey.h
             src/main/jni/transition/PeerManager.c
             src/main/jni/transition/PeerManager.h
             src/main/jni/transition/wallet.c
             src/main/jni/transition/wallet.h)


include_directories(src/main/jni/bitwallet-core/)
include_directories(src/main/secp/secp256k1/include/)
include_directories(src/main/secp/secp256k1/src/)
include_directories(src/main/secp/secp256k1/)
include_directories(src/main/secp/)

# add lib dependencies
target_link_libraries(
    bitw-core-lib
    android
    log)
