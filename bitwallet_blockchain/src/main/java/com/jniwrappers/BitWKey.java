package com.jniwrappers;

import java.util.Arrays;

/**
 * BreadWallet
 * <p/>
 * Created by Mihail Gutan on <mihail@bitwallet.com> 10/9/16.
 * Copyright (c) 2016 bitwallet LLC
 * <p/>
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * <p/>
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * <p/>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
public class BitWKey {
    public static final String TAG = BitWKey.class.getName();

    public BitWKey(byte[] key, int coinType) throws IllegalArgumentException {
        if (isNullOrEmpty(key)) throw new NullPointerException("key is empty");
        if (!setPrivKey(key, coinType)) {
            throw new IllegalArgumentException("Failed to setup the key: " + Arrays.toString(key));
        }
    }

    public BitWKey(String hexSecret) {
        setSecret(hexToBytes(hexSecret));
    }


    public static boolean isNullOrEmpty(byte[] arr) {
        return arr == null || arr.length == 0;
    }

    public static byte[] hexToBytes(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    private native boolean setPrivKey(byte[] privKey, int coinType);

    private native void setSecret(byte[] secret);

    public native byte[] compactSign(byte[] data);

    public native byte[] encryptNative(byte[] data, byte[] nonce);

    public native byte[] decryptNative(byte[] data, byte[] nonce);

    public native String address();

}
