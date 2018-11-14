package com.bitwallet.blockchain.eth;

import com.bitwallet.blockchain.tools.BlockChainLog;
import com.bitwallet.blockchain.tools.BytesUtil;

import org.ethereum.geth.Geth;
import org.ethereum.geth.Transaction;

import java.math.BigInteger;

/**
 * Created by zhoubin on 2018/4/13.
 * https://github.com/AElfProject/ethereum_scanner/blob/master/main.go
 * if len(input) > 74 && string(input[:10]) == transfer {
 //			log.Println(string(input))
 tx.MethodId = string(input[:10])

 tx.AddrToken = string(append([]byte{'0', 'x'}, input[34:74]...))
 tx.TokenValue.UnmarshalJSON(append([]byte{'0', 'x'}, input[74:]...))
 tx.Type = Token
 }
 */

public class EthTransaction {
    private static final String TAG = EthTransaction.class.getSimpleName();
    Transaction mTransaction;
    private String methodId;
    private String mToTokenAddress;
    private BigInteger mTokenValue;
    private boolean mIsToken = false;

    private static final String TRANSFER = "0xa9059cbb";
    public static EthTransaction decodeTransaction(byte[] unsignedTx) {
        EthTransaction ethTransaction = new EthTransaction();
        try {
            ethTransaction.setTransaction(Geth.newTransactionFromRLP(unsignedTx));
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
        return ethTransaction;
    }

    private void setTransaction(Transaction transaction){
        mTransaction = transaction;
        String inputData = "0x" + BytesUtil.bytesToHex(transaction.getData()).toLowerCase();

        if(inputData.length()>74&&inputData.startsWith(TRANSFER))
        {
            methodId = inputData.substring(0, 10);
            mToTokenAddress = "0x" + inputData.substring(34,74);
            mTokenValue = new BigInteger(BytesUtil.hexToBytes(inputData.substring(74)));
            mIsToken = true;
        }
        BlockChainLog.d(TAG, "input:" + inputData);
        BlockChainLog.d(TAG, "methodId:" + methodId);
        BlockChainLog.d(TAG, "mToTokenAddress:" + mToTokenAddress);
        BlockChainLog.d(TAG, "mTokenValue:" + mTokenValue);
    }

    public BigInteger getTokenValue(){
        return mTokenValue;
    }
    public boolean isToken(){
        return mIsToken;
    }
    /**
     * 代币转账地址
     * @return
     */
    public String getToTokenAddress(){
        return mToTokenAddress;
    }

    public String getToAddress() {
        if(mTransaction!= null)
        {
            return "0x" +  BytesUtil.bytesToHex(mTransaction.getTo().getBytes()).toLowerCase();
        }
        return null;
    }

    public long getFee(){
        if(mTransaction!=null)
        {
            long gas = mTransaction.getGas();
            long gasPrice = mTransaction.getGasPrice().getInt64();
            return gas * gasPrice;
        }
        return 0;
    }

    public BigInteger getCost(){
        if(mTransaction != null)
        {
            return new BigInteger(mTransaction.getCost().toString());
        }
        return new BigInteger("0");
    }
    public Transaction getTx() {
        return mTransaction;
    }
}
