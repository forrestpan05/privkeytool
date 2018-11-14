package com.bitwallet.blockchain.tools.security;

import com.bitwallet.blockchain.eth.EthAddressEncoder;
import com.bitwallet.blockchain.model.AddressEncoder;
import com.bitwallet.blockchain.model.CoinType;
import com.jniwrappers.BitWallet;

import java.io.IOException;

public class AddressHelper {
    private static final String TAG = AddressHelper.class.getName();


    public static boolean isValid(CoinType coinType, String url) {
        switch (coinType) {
            case ETH: {
                try {
                    EthAddressEncoder encoder = EthAddressEncoder.decode(url);
                    String address = encoder.address.toLowerCase();
                    return address.startsWith("0x") && address.length() == EthAddressEncoder.ADDRESS_LENGTH_IN_HEX;
                } catch (IOException e) {
                    e.printStackTrace();
                    return false;
                }
            }
            default: {
                return isValidBtcGroup(coinType, url);
            }
        }
    }

    private static boolean isValidBtcGroup(CoinType coinType, String url) {
        try {
            AddressEncoder requestObject = AddressEncoder.decode(coinType, url);
            // return true if the request is valid url and has param: r or param: address
            // return true if it is a valid bitcoinPrivKey
            return (requestObject != null && (requestObject.r != null || requestObject.address != null)
                    || BitWallet.getInstance().isValidBitcoinBIP38Key(url)
                    || BitWallet.getInstance().isValidBitcoinPrivateKey(url, coinType.getBip44Index()));
        } catch (Exception e) { // Illegal character
            e.printStackTrace();
        }
        return false;
    }
}
