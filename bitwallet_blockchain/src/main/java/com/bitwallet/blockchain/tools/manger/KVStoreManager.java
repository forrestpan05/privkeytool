package com.bitwallet.blockchain.tools.manger;

/**
 * Created by zhoubin on 2018/3/7.
 */

public class KVStoreManager {
    private static final String TAG = KVStoreManager.class.getSimpleName();


    private static KVStoreManager instance;


    private KVStoreManager() {
    }

    public static KVStoreManager getInstance() {
        if (instance == null) instance = new KVStoreManager();
        return instance;
    }


}
