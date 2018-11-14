package com.bitwallet.blockchain.bch.cashaddr;

import com.bitwallet.blockchain.BuildConfig;
import com.bitwallet.blockchain.bch.BchUtils;

import java.math.BigInteger;
import java.util.Arrays;


/**
 * Copyright (c) 2018 Tobias Brandt
 * <p>
 * Distributed under the MIT software license, see the accompanying file LICENSE
 * or http://www.opensource.org/licenses/mit-license.php.
 */
public class BitcoinCashAddressFormatter {

    public static final String SEPARATOR = ":";

    public static final String MAIN_NET_PREFIX = "bitcoincash";

    public static final String TEST_NET_PREFIX = "bchtest";

    public static final String ASSUMED_DEFAULT_PREFIX = MAIN_NET_PREFIX;

    private static final BigInteger[] POLYMOD_GENERATORS = new BigInteger[]{new BigInteger("98f2bc8e61", 16),
            new BigInteger("79b76d99e2", 16), new BigInteger("f33e5fb3c4", 16), new BigInteger("ae2eabe2a8", 16),
            new BigInteger("1e4f43e470", 16)};

    private static final BigInteger POLYMOD_AND_CONSTANT = new BigInteger("07ffffffff", 16);

    public static String toCashAddress(BitcoinCashAddressType addressType, byte[] hash) {
        String prefixString = getPrefixString();
        byte[] prefixBytes = getPrefixBytes(prefixString);
        byte[] payloadBytes = BchUtils.concatenateByteArrays(new byte[]{addressType.getVersionByte()}, hash);
        payloadBytes = BitcoinCashBitArrayConverter.convertBits(payloadBytes, 8, 5, false);
        byte[] allChecksumInput = BchUtils.concatenateByteArrays(
                BchUtils.concatenateByteArrays(BchUtils.concatenateByteArrays(prefixBytes, new byte[]{0}), payloadBytes),
                new byte[]{0, 0, 0, 0, 0, 0, 0, 0});
        byte[] checksumBytes = calculateChecksumBytesPolymod(allChecksumInput);
        checksumBytes = BitcoinCashBitArrayConverter.convertBits(checksumBytes, 8, 5, true);
        String cashAddress = BitcoinCashBase32.encode(BchUtils.concatenateByteArrays(payloadBytes, checksumBytes));
        return prefixString + SEPARATOR + cashAddress;
    }

    public static BitcoinCashAddressDecodedParts decodeCashAddress(String bitcoinCashAddress) {
        if (!isValidCashAddress(bitcoinCashAddress)) {
            throw new RuntimeException("Address wasn't valid: " + bitcoinCashAddress);
        }

        BitcoinCashAddressDecodedParts decoded = new BitcoinCashAddressDecodedParts();
        String[] addressParts = bitcoinCashAddress.split(SEPARATOR);
        if (addressParts.length == 2) {
            decoded.setPrefix(addressParts[0]);
        }

        byte[] addressData = BitcoinCashBase32.decode(addressParts[1]);
        addressData = Arrays.copyOfRange(addressData, 0, addressData.length - 8);
        addressData = BitcoinCashBitArrayConverter.convertBits(addressData, 5, 8, true);
        byte versionByte = addressData[0];
        byte[] hash = Arrays.copyOfRange(addressData, 1, addressData.length);

        decoded.setAddressType(getAddressTypeFromVersionByte(versionByte));
        decoded.setHash(hash);

        return decoded;
    }

    private static BitcoinCashAddressType getAddressTypeFromVersionByte(byte versionByte) {
        for (BitcoinCashAddressType addressType : BitcoinCashAddressType.values()) {
            if (addressType.getVersionByte() == versionByte) {
                return addressType;
            }
        }

        throw new RuntimeException("Unknown version byte: " + versionByte);
    }

    public static boolean isValidCashAddress(String bitcoinCashAddress) {
        try {
            boolean isTestNet = BuildConfig.BITCOIN_TESTNET;
            String prefix;
            if (bitcoinCashAddress.contains(SEPARATOR)) {
                String[] split = bitcoinCashAddress.split(SEPARATOR);
                prefix = split[0];
                bitcoinCashAddress = split[1];

                if (MAIN_NET_PREFIX.equals(prefix) && isTestNet) {//测试网络 主网前缀
                    return false;
                }
                if (TEST_NET_PREFIX.equals(prefix) && !isTestNet) {//正式网络 测试前缀
                    return false;
                }
            } else {
                prefix = isTestNet ? TEST_NET_PREFIX : MAIN_NET_PREFIX;
            }

            if (!isSingleCase(bitcoinCashAddress))
                return false;

            bitcoinCashAddress = bitcoinCashAddress.toLowerCase();

            byte[] checksumData = BchUtils.concatenateByteArrays(
                    BchUtils.concatenateByteArrays(getPrefixBytes(prefix), new byte[]{0x00}),
                    BitcoinCashBase32.decode(bitcoinCashAddress));

            byte[] calculateChecksumBytesPolymod = calculateChecksumBytesPolymod(checksumData);
            return new BigInteger(calculateChecksumBytesPolymod).compareTo(BigInteger.ZERO) == 0;
        } catch (RuntimeException re) {
            return false;
        }
    }

    private static boolean isSingleCase(String bitcoinCashAddress) {
        if (bitcoinCashAddress.equals(bitcoinCashAddress.toLowerCase())) {
            return true;
        }
        if (bitcoinCashAddress.equals(bitcoinCashAddress.toUpperCase())) {
            return true;
        }

        return false;
    }

    /**
     * @param checksumInput
     * @return Returns a 40 bits checksum in form of 5 8-bit arrays. This still has
     * to me mapped to 5-bit array representation
     */
    private static byte[] calculateChecksumBytesPolymod(byte[] checksumInput) {
        BigInteger c = BigInteger.ONE;

        for (int i = 0; i < checksumInput.length; i++) {
            byte c0 = c.shiftRight(35).byteValue();
            c = c.and(POLYMOD_AND_CONSTANT).shiftLeft(5)
                    .xor(new BigInteger(String.format("%02x", checksumInput[i]), 16));

            if ((c0 & 0x01) != 0)
                c = c.xor(POLYMOD_GENERATORS[0]);
            if ((c0 & 0x02) != 0)
                c = c.xor(POLYMOD_GENERATORS[1]);
            if ((c0 & 0x04) != 0)
                c = c.xor(POLYMOD_GENERATORS[2]);
            if ((c0 & 0x08) != 0)
                c = c.xor(POLYMOD_GENERATORS[3]);
            if ((c0 & 0x10) != 0)
                c = c.xor(POLYMOD_GENERATORS[4]);
        }

        byte[] checksum = c.xor(BigInteger.ONE).toByteArray();
        if (checksum.length == 5) {
            return checksum;
        } else {
            byte[] newChecksumArray = new byte[5];

            System.arraycopy(checksum, Math.max(0, checksum.length - 5), newChecksumArray,
                    Math.max(0, 5 - checksum.length), Math.min(5, checksum.length));

            return newChecksumArray;
        }

    }

    private static byte[] getPrefixBytes(String prefixString) {
        byte[] prefixBytes = new byte[prefixString.length()];

        char[] charArray = prefixString.toCharArray();
        for (int i = 0; i < charArray.length; i++) {
            prefixBytes[i] = (byte) (charArray[i] & 0x1f);
        }

        return prefixBytes;
    }

    private static String getPrefixString() {
        if (BuildConfig.BITCOIN_TESTNET) {
            return TEST_NET_PREFIX;
        } else {
            return MAIN_NET_PREFIX;

        }
    }

}
