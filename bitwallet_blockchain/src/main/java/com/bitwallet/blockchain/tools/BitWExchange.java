package com.bitwallet.blockchain.tools;

import android.text.TextUtils;

import java.math.BigDecimal;
import java.math.RoundingMode;
import java.text.DecimalFormat;

/**
 * Created by zhoubin on 2018/3/12.
 * 货币转换
 */
public class BitWExchange {

    private static final String TAG = BitWExchange.class.getName();

    /**
     *
     */
    public static final BigDecimal DEFAULT_ONE_COIN = new BigDecimal(100_000_000);
    /**
     * 最大的小数位数
     */
    private static final int DEFAULT_MAX_FRACTIONDIGITS = 5;

    public static final int MAX_FRACTIONDIGITS_8 = 8;
    public static final String DECIMAL_PATTERN_DEFAULT = "#0.00000";
    public static final String DECIMAL_PATTERN_SCALE0 = "#0";
    public static final String DECIMAL_PATTERN_SCALE8 = "#0.00000000";


    /**
     * coin格式化
     *
     * @param strAmount
     * @return
     */
    public static String getFormatCoinWithUnitStrValue(String strAmount, BigDecimal oneCoinUnit) {
        return getFormatCoinWithUnitStrValue( strAmount,oneCoinUnit, false);
    }


    public static String getFormatCoinWithUnitStrValue(String strAmount, BigDecimal oneCoinUnit, boolean addPlusFlg) {
        if (TextUtils.isEmpty(strAmount)) {
            return strAmount;
        }
        BigDecimal originalAmount = new BigDecimal(strAmount);
        BigDecimal btnAmount = getAmountFromUnit(originalAmount,oneCoinUnit, DEFAULT_MAX_FRACTIONDIGITS);
        String result = decimalFormat(btnAmount, DECIMAL_PATTERN_DEFAULT);

        if (addPlusFlg) {
            if (originalAmount.compareTo(new BigDecimal(0)) > 0) {
                result = "+" + result;
            }
        }
        return result;
    }

    public static String decimalFormat(BigDecimal bigDecimal, String pattern) {
        DecimalFormat decimalFormat = new DecimalFormat(pattern);
        decimalFormat.setRoundingMode(RoundingMode.DOWN);
        return decimalFormat.format(bigDecimal);
    }

    public static String getFormatCoinWithUnitLongValue(long amount, BigDecimal oneCoinUnit) {
        BigDecimal btnAmount = getAmountFromUnit(new BigDecimal(amount), oneCoinUnit, DEFAULT_MAX_FRACTIONDIGITS);
        return btnAmount.toString();
    }


    public static BigDecimal getAmountFromUnit(BigDecimal amountUnit, BigDecimal oneCoinUnit, int maxFractionDigits) {
        BigDecimal result = null;
        if (amountUnit.compareTo(BigDecimal.ZERO) == 0) {
            result = BigDecimal.ZERO;
        } else {
            result = new BigDecimal(String.valueOf(amountUnit));
        }

        BigDecimal unit = oneCoinUnit;
        if(oneCoinUnit.compareTo(BigDecimal.ZERO) == 0)
        {
            unit = DEFAULT_ONE_COIN;
        }
        return result.divide(unit).setScale(maxFractionDigits, RoundingMode.DOWN);
    }

    /**
     * 转换为最小单位
     *
     * @param inputCoin
     * @return
     */

    public static BigDecimal getUnitFromAmount(BigDecimal oneCoinUnit, BigDecimal inputCoin) {
        BigDecimal result = inputCoin.multiply(oneCoinUnit);
        return result;
    }


}