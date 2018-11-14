package com.bitwallet.blockchain.eth;

import com.bitwallet.blockchain.model.AddressEncoder;

import java.io.IOException;
import java.math.BigInteger;

public class EthAddressEncoder extends AddressEncoder{
    public static final int ADDRESS_LENGTH_IN_HEX = 42;
    private String gas;
    private String data;
    private byte type;

    public EthAddressEncoder(String address, String amount) {
        this(address);
        this.amount = amount;
    }

    public EthAddressEncoder(String address) {
        this.address = address;
    }

    public static EthAddressEncoder decode(String s) throws IOException {
        if (s.startsWith("ethereum:") || s.startsWith("ETHEREUM:"))
            return decodeERC(s);
        else if (s.startsWith("iban:XE") || s.startsWith("IBAN:XE"))
            return decodeICAP(s);
        else
            return decodeLegacyLunary(s);
    }

    public static EthAddressEncoder decodeERC(String s) throws IOException {
        if (!s.startsWith("ethereum:") && !s.startsWith("ETHEREUM:"))
            throw new IOException("Invalid data format, see ERC-67 https://github.com/ethereum/EIPs/issues/67");
        EthAddressEncoder re = new EthAddressEncoder(s.substring(9, 51));
        if(s.length() == 51) return re;
        String[] parsed = s.substring(51).split("\\?");
        for (String entry : parsed) {
            String[] entry_s = entry.split("=");
            if (entry_s.length != 2) continue;
            if (entry_s[0].equalsIgnoreCase("value")) re.amount = entry_s[1];
            if (entry_s[0].equalsIgnoreCase("gas")) re.gas = entry_s[1];
            if (entry_s[0].equalsIgnoreCase("data")) re.data = entry_s[1];
        }
        return re;
    }

    public static String encodeERC(EthAddressEncoder a) {
        String re = "ethereum:" + a.address;
        if (a.amount != null) re += "?value=" + a.amount;
        if (a.gas != null) re += "?gas=" + a.gas;
        if (a.data != null) re += "?data=" + a.data;
        return re;
    }

    public static EthAddressEncoder decodeICAP(String s) throws IOException {
        if (!s.startsWith("iban:XE") && !s.startsWith("IBAN:XE"))
            throw new IOException("Invalid data format, see ICAP https://github.com/ethereum/wiki/wiki/ICAP:-Inter-exchange-Client-Address-Protocol");
        // TODO: verify checksum and length
        String temp = s.substring(9);
        int index = temp.indexOf("?") > 0 ? temp.indexOf("?") : temp.length();
        String address = new BigInteger(temp.substring(0, index), 36).toString(16);
        while (address.length() < 40)
            address = "0" + address;
        EthAddressEncoder re = new EthAddressEncoder("0x" + address);
        String[] parsed = s.split("\\?");
        for (String entry : parsed) {
            String[] entry_s = entry.split("=");
            if (entry_s.length != 2) continue;
            if (entry_s[0].equalsIgnoreCase("amount")) re.amount = entry_s[1];
            if (entry_s[0].equalsIgnoreCase("gas")) re.gas = entry_s[1];
            if (entry_s[0].equalsIgnoreCase("data")) re.data = entry_s[1];
        }
        return re;
    }

    public static EthAddressEncoder decodeLegacyLunary(String s) throws IOException {
        if (!s.startsWith("iban:") && !s.startsWith("IBAN:")) return new EthAddressEncoder(s);
        String temp = s.substring(5);
        String amount = null;
        if (temp.indexOf("?") > 0) {
            if (temp.indexOf("amount=") > 0 && temp.indexOf("amount=") < temp.length())
                amount = temp.substring(temp.indexOf("amount=") + 7);
            temp = temp.substring(0, temp.indexOf("?"));
        }
        EthAddressEncoder re = new EthAddressEncoder(temp);
        re.setAmount(amount);
        return re;
    }


    public String getGas() {
        return gas;
    }

    public void setGas(String gas) {
        this.gas = gas;
    }


    public String getData() {
        return data;
    }

    public void setData(String data) {
        this.data = data;
    }

    public byte getType() {
        return type;
    }

    public void setType(byte type) {
        this.type = type;
    }
}
