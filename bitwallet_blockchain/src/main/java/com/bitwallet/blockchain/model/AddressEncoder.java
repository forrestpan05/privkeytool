package com.bitwallet.blockchain.model;


import com.jniwrappers.BitWallet;

import java.math.BigDecimal;
import java.net.URI;

/**
 * 比特币系列
 */
public class AddressEncoder {
    public static final String TAG = AddressEncoder.class.getName();
    public String address;
    public String r;
    public String amount;
    public String label;
    public String message;
    public String req;


    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        this.address = address;
    }

    public String getAmount() {
        return amount;
    }

    public void setAmount(String amount) {
        this.amount = amount;
    }


    public static AddressEncoder decode(CoinType coinType, String str) {
        if (str == null || str.isEmpty()) return null;
        AddressEncoder obj = new AddressEncoder();

        String tmp = str.trim().replaceAll("\n", "").replaceAll(" ", "%20");

        String uriScheme = coinType.getScheme() + "://";
        String coinScheme = coinType.getScheme() + ":";


        if (!tmp.startsWith(uriScheme)) {
            if (!tmp.startsWith(coinScheme))
                tmp = uriScheme.concat(tmp);
            else
                tmp = tmp.replace(coinScheme, uriScheme);
        }
        URI uri = URI.create(tmp);

        String host = uri.getHost();
        if (host != null) {
            String addrs = host.trim();
            if (BitWallet.validateAddress(addrs, coinType.getBip44Index())) {
                obj.address = addrs;
            }
        }
        String query = uri.getQuery();
        if (query == null) return obj;
        String[] params = query.split("&");
        for (String s : params) {
            String[] keyValue = s.split("=", 2);
            if (keyValue.length != 2)
                continue;
            if (keyValue[0].trim().equals("amount")) {
                try {
                    BigDecimal bigDecimal = new BigDecimal(keyValue[1].trim());
//                    obj.amount = bigDecimal.multiply(new BigDecimal(coinType.getOneCoin())).toString();
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                }
            } else if (keyValue[0].trim().equals("label")) {
                obj.label = keyValue[1].trim();
            } else if (keyValue[0].trim().equals("message")) {
                obj.message = keyValue[1].trim();
            } else if (keyValue[0].trim().startsWith("req")) {
                obj.req = keyValue[1].trim();
            } else if (keyValue[0].trim().startsWith("r")) {
                obj.r = keyValue[1].trim();
            }
        }
        return obj;
    }

}
