package com.jniwrappers;

import android.content.Context;
import android.util.Log;

import com.bitwallet.blockchain.model.CoinType;
import com.bitwallet.blockchain.tools.Bip39Reader;
import com.bitwallet.blockchain.tools.manger.BitWReportsManager;

import java.util.List;
import java.util.Locale;

/**
 * https://github.com/satoshilabs/slips/blob/master/slip-0044.md
 * https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
 * https://iancoleman.io/bip39/#english
 * Created by zhoubin on 2018/3/9.
 * 助记词
 */

public class MnemonicCode {
    private static final String TAG = MnemonicCode.class.getSimpleName();
    public static final String BIP44_PATH_ETH = "m/44'/60'/0'/0/0";


    /**
     * Constant 0 is used for external chain and constant 1 for internal chain (also known as change addresses).
     * External chain is used for addresses that are meant to be visible outside of the wallet (e.g. for receiving payments).
     * Internal chain is used for addresses which are not meant to be visible outside of the wallet and is used for return transaction change.
     */
    public static final int EXTERNAL_CHANGE = 0;//收款地址
    public static final int INTERNAL_CHANGE = 1;//找零地址

    private static MnemonicCode instance;

    private MnemonicCode() {
    }

    public static MnemonicCode getInstance() {
        if (instance == null) {
            instance = new MnemonicCode();
        }
        return instance;
    }

    /**
     * 助记词 校验
     * @param ctx
     * @param paperKey
     * @return
     */
    public static boolean isPaperKeyValid(Context ctx, String paperKey) {
        String languageCode = Locale.getDefault().getLanguage();
        if (!isValid(ctx, paperKey, languageCode)) {
            //try all langs
            for (String lang : Bip39Reader.LANGS_BIP39) {
                if (isValid(ctx, paperKey, lang)) {
                    return true;
                }
            }
        } else {
            return true;
        }

        return false;

    }



    private static boolean isValid(Context ctx, String paperKey, String lang) {
        List<String> list = Bip39Reader.bip39List(ctx, lang);
        String[] words = list.toArray(new String[list.size()]);
        if (words.length % Bip39Reader.WORD_LIST_SIZE != 0) {
            Log.e(TAG, "isPaperKeyValid: " + "The list size should divide by " + Bip39Reader.WORD_LIST_SIZE);
            BitWReportsManager.reportBug(new IllegalArgumentException("words.length is not dividable by " + Bip39Reader.WORD_LIST_SIZE), true);
        }
        boolean result = BitWallet.getInstance().validateRecoveryPhrase(words, paperKey);
        return result;
    }


    public native byte[] encodeSeed(byte[] seed, String[] wordList);

    public static native byte[] getSeedFromPhrase(byte[] phrase);

    public static byte[] getFirstPrivKeyByCoinType(byte[] seed, int coinType) {
        return getPrivKeyByCoinType(seed, coinType, EXTERNAL_CHANGE, 0);
    }

    public static native byte[] getPrivKeyByCoinType(byte[] seed, int coinType, int change, int index);

    public static native byte[] getAuthPrivKeyForAPI(byte[] seed);

    public static byte[] getMasterPubKey(byte[] bytePhrase, CoinType coinType)
    {
        return  getMasterPubKey(bytePhrase, coinType.getBip44Index());
    }
    public static native byte[] getMasterPubKey(byte[] bytePhrase, int coinType);


//    public MnemonicCode(InputStream wordstream, String wordListDigest) throws IOException, IllegalArgumentException {
//        BufferedReader br = new BufferedReader(new InputStreamReader(wordstream, "UTF-8"));
//        this.wordList = new ArrayList<String>(2048);
//        MessageDigest md = Sha256Hash.newDigest();
//        String word;
//        while ((word = br.readLine()) != null) {
//            md.update(word.getBytes());
//            this.wordList.add(word);
//        }
//        br.close();
//
//        if (this.wordList.size() != 2048)
//            throw new IllegalArgumentException("input stream did not contain 2048 words");
//
//        // If a wordListDigest is supplied check to make sure it matches.
//        if (wordListDigest != null) {
//            byte[] digest = md.digest();
//            String hexdigest = HEX.encode(digest);
//            if (!hexdigest.equals(wordListDigest))
//                throw new IllegalArgumentException("wordlist digest mismatch");
//        }
//    }
}
