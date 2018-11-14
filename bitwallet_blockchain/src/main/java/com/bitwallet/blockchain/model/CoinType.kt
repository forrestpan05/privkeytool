package com.bitwallet.blockchain.model

import com.jniwrappers.BitWallet

/**
 * Created by zhoubin on 2018/3/12.
 */

enum class CoinType(var coinCode: String,
                    var scheme: String,
                    var bip44Index: Int,
                    var unit:String,
                    var maxFractionDigits: Int//最大小数位
) {
    BTC("BTC", "bitcoin", BitWallet.BIP44_COIN_INDEX_BTC, "100000000", 5),
    BCH("BCH", "bitcoin", BitWallet.BIP44_COIN_INDEX_BCH, "100000000", 5),
    BTN("BTN", "btn", BitWallet.BIP44_COIN_INDEX_BTN, "100000000", 5),
    QTUM("QTUM", "qtum", BitWallet.BIP44_COIN_INDEX_QTUM, "100000000", 5),
    ETH("ETH", "ethereum", BitWallet.BIP44_COIN_INDEX_ETH, "1000000000000000000", 5),
    EOS("EOS", "eos", BitWallet.BIP44_COIN_INDEX_EOS, "1000000000000000000", 5),
    UNKNOW("UNKNOW", "forrest", -1, "100000000", 5);


    companion object {
        //TODO 默认返回UNKONW
        @JvmStatic
        fun coinOf(coinCode: String?) = CoinType.values().firstOrNull { it.coinCode.equals(coinCode, ignoreCase = true) }
                ?: UNKNOW

        fun isValid(coinType: CoinType) = coinType != UNKNOW
    }
}
