package com.bitwallet.blockchain.tools.manger;

import android.util.Log;
public class BitWReportsManager {

    private static final String TAG = BitWReportsManager.class.getName();

    public static void reportBug(RuntimeException er, boolean crash) {
        Log.e(TAG, "reportBug: ", er);
        try {
//            FirebaseCrash.report(er);
        } catch (Exception e) {
            Log.e(TAG, "reportBug: failed to report to FireBase: ", e);
        }
        if (crash) throw er;
    }

    public static void reportBug(Exception er) {
        Log.e(TAG, "reportBug: ", er);
        try {
//            FirebaseCrash.report(er);
        } catch (Exception e) {
            Log.e(TAG, "reportBug: failed to report to FireBase: ", e);
        }
    }
}
