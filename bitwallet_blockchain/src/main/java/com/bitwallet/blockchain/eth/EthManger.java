package com.bitwallet.blockchain.eth;

import com.bitwallet.blockchain.BuildConfig;
import com.bitwallet.blockchain.model.CoinType;
import com.bitwallet.blockchain.tools.BlockChainLog;

import org.ethereum.geth.BigInt;
import org.ethereum.geth.Geth;
import org.ethereum.geth.Transaction;

import static com.jniwrappers.BitWallet.getStrFirstPrivKey;


/**
 * Created by zhoubin on 2018/3/26.
 * https://github.com/ethereum/go-ethereum/blob/762f3a48a00da02fe58063cb6ce8dc2d08821f15/mobile/android_test.go
 * https://github.com/ethereum/go-ethereum/wiki/Mobile:-Account-management
 */

public class EthManger {
    private static final String TAG = EthManger.class.getSimpleName();
    public static String getAddress(byte[] nullTermPhrase) {
        try {
            final String privateKey = getStrFirstPrivKey(nullTermPhrase, CoinType.ETH.getBip44Index());
            return Geth.getAddress(privateKey).toLowerCase();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        return "";
    }

    public static void dumpTransaction(Transaction transaction){
        if(BuildConfig.DEBUG_ENABLE&&transaction!=null)
        {
            try {
                BlockChainLog.d(TAG, "transaction:" + transaction.encodeJSON());

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public static byte[] signTransaction(byte[] nullTermPhrase, String toAddress, byte[] unsignedTx) {
        try {
            EthTransaction transaction = EthTransaction.decodeTransaction(unsignedTx);
            String transToAddress;
            if(transaction.isToken())//token
            {
                transToAddress = transaction.getToTokenAddress();
            }else
            {
                transToAddress = transaction.getToAddress();
            }

            if(toAddress.compareToIgnoreCase(transToAddress)!=0)
            {
                return null;
            }

            final BigInt chain = BuildConfig.BITCOIN_TESTNET ? new BigInt(4) : new BigInt(1);
            final String privateKey = getStrFirstPrivKey(nullTermPhrase, CoinType.ETH.getBip44Index());
            return Geth.signTx(transaction.getTx(), chain, privateKey).encodeRLP();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
