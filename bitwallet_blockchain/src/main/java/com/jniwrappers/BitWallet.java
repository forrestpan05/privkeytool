package com.jniwrappers;


import com.bitwallet.blockchain.tools.BytesUtil;

import static com.jniwrappers.MnemonicCode.EXTERNAL_CHANGE;

/**
 * Created by zhoubin on 2018/3/7.
 */

public class BitWallet {


    //    m / purpose' / coin_type' / account' / change / address_index
    /**
     * https://github.com/satoshilabs/slips/blob/master/slip-0044.md
     */
    public static final int BIP44_COIN_INDEX_BTC = 0;
    public static final int BIP44_COIN_INDEX_BCH = 145;
    public static final int BIP44_COIN_INDEX_BTN = 1000;
    public static final int BIP44_COIN_INDEX_ETH = 60;
    public static final int BIP44_COIN_INDEX_EOS = 194;

    public static final int BIP44_COIN_INDEX_QTUM = 2301;

    private static BitWallet instance;

    private BitWallet() {
    }

    public static BitWallet getInstance() {
        if (instance == null) {
            instance = new BitWallet();
        }
        return instance;
    }



    public static String getStrFirstPrivKey(byte[] nullTermPhrase, int coinType) {
        return new String(getFirstPrivKey(nullTermPhrase, coinType));
    }

    public static byte[] getFirstPrivKey(byte[] nullTermPhrase, int coinType) {
        return getPrivKey(nullTermPhrase, coinType, EXTERNAL_CHANGE, 0);
    }

    public static byte[] getPrivKey(byte[] nulTermPhrase, int coinType, int change, int index) {
        byte[] privKey = MnemonicCode.getPrivKeyByCoinType(getSeed(nulTermPhrase), coinType, change, index);
        return privKey;
    }

    public static byte[] getSeed(byte[] nulTermPhrase) {
        if (BytesUtil.isNullOrEmpty(nulTermPhrase)) {
            throw new NullPointerException("phrase cannot be empty");
        }

//        byte[] nullTerminatedPhrase = TypesConverter.getNullTerminatedPhrase(phrase);
        byte[] seed = MnemonicCode.getSeedFromPhrase(nulTermPhrase);
        return seed;
    }

    public native void createWallet(int transactionCount, byte[] pubkey);

    public native void putTransaction(byte[] transaction, long blockHeight, long timeStamp);

    public native void createTxArrayWithCount(int count);

    private static final String ADDRESS_EOS_PREFIX = "EOS";
    public static String getFirstAddress(byte[] mpk, int coinType)
    {
        switch (coinType)
        {
            case BIP44_COIN_INDEX_EOS:
            {
                return ADDRESS_EOS_PREFIX + getAddress(mpk, coinType, EXTERNAL_CHANGE, 0);
            }
            default:
            {
                return getAddress(mpk, coinType, EXTERNAL_CHANGE, 0);
            }
        }

    }

    public native static String getAddress(byte[] mpk, int coinType, int change, int index);

    public static native boolean validateAddress(String address, int coinType);

    public native boolean addressContainedInWallet(String address);

    public native boolean addressIsUsed(String address);

    public native int feeForTransaction(String addressHolder, long amountHolder);

    public native int feeForTransactionAmount(long amountHolder);

    public native long getMinOutputAmount();

    public native long getMaxOutputAmount();

    public native boolean isCreated();

    public native byte[] tryTransaction(String addressHolder, long amountHolder);

    // returns the given amount (amount is in satoshis) in local currency units (i.e. pennies, pence)
    // price is local currency units per bitcoin
    public native long localAmount(long amount, double price);

    // returns the given local currency amount in satoshis
    // price is local currency units (i.e. pennies, pence) per bitcoin
    public native long bitcoinAmount(long localAmount, double price);

    public native void walletFreeEverything();

    public native boolean validateRecoveryPhrase(String[] words, String phrase);



    public native byte[] publishSerializedTransaction(byte[] serializedTransaction, byte[] nullTermPhrase);

    /**
     * 签名
     *
     * @param serializedTransaction
     * @param nullTermPhrase TypesConverter.getNullTerminatedPhrase
     * @return
     */
    public static native byte[] signTransaction(int coinType, byte[] serializedTransaction, byte[] nullTermPhrase, String toAddress, String[] addressList, long[] amountList);

    public native long getTotalSent();

    public native long setFeePerKb(long fee, boolean ignore);

    public native boolean isValidBitcoinPrivateKey(String key, int coinType);

    public native boolean isValidBitcoinBIP38Key(String key);

    public native String getAddressFromPrivKey(String key);

    public native void createInputArray();

    public native void addInputToPrivKeyTx(byte[] hash, int vout, byte[] script, long amount);

    public native boolean confirmKeySweep(byte[] tx, String key);

    public native String decryptBip38Key(String privKey, String pass);

    public native String reverseTxHash(String txHash);

    public native String txHashToHex(byte[] txHash);

//    public native String txHashSha256Hex(String txHash);

    public native long nativeBalance();

    public native int getTxCount();

    public native long getMinOutputAmountRequested();

    public static native String uncompressPrivKey(byte[] privKey, int coinType);

    public static native String getAuthPublicKeyForAPI(byte[] privKey);



    public static native boolean isTestNet();

    public static native byte[] sweepBCash(byte[] pubKey, String address, byte[] nullTermPhrase);


    public static native long getBCashBalance(byte[] pubKey);

    public static native int getTxSize(byte[] serializedTx);
}
