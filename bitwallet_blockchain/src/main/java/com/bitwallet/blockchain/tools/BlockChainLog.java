/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.bitwallet.blockchain.tools;

import android.util.Log;

import com.bitwallet.blockchain.BuildConfig;

import java.util.Locale;

public class BlockChainLog {

    public static final int LEVEL_VERBOSE = Log.VERBOSE;
    public static final int LEVEL_DEBUG = Log.DEBUG;
    public static final int LEVEL_INFO = Log.INFO;
    public static final int LEVEL_WARN = Log.WARN;
    public static final int LEVEL_ERROR = Log.ERROR;


    private static int getLevel(){
        if(BuildConfig.DEBUG_ENABLE)
        {
            return LEVEL_VERBOSE;
        }else
        {
            return LEVEL_ERROR;
        }
    }
    public static void v(String tag, String format, Object... args) {
        if (getLevel() > LEVEL_VERBOSE) {
            return;
        }
        Log.v(tag, buildMessage(tag, format, args));
    }


    public static void d(String tag, String msg) {
        if (getLevel() > LEVEL_DEBUG) {
            return;
        }
        Log.d(tag, buildMessage(tag, "%s", msg));
    }

    public static void e(String tag, String msg) {
        if (getLevel() > LEVEL_DEBUG) {
            return;
        }
        Log.e(tag, buildMessage(tag, "%s", msg));
    }

    public static void d(String tag, String format, Object... args) {
        if (getLevel() > LEVEL_DEBUG) {
            return;
        }
        Log.d(tag, buildMessage(tag, format, args));
    }

    public static void i(String tag, String msg) {
        if (getLevel() > LEVEL_DEBUG) {
            return;
        }
        Log.i(tag, buildMessage(tag, "%s", msg));
    }
    public static void i(String tag, String format, Object... args) {
        if (getLevel() > LEVEL_INFO) {
            return;
        }
        Log.i(tag, buildMessage(tag, format, args));
    }

    public static void w(String tag, String format, Object... args) {
        if (getLevel() > LEVEL_WARN) {
            return;
        }
        Log.w(tag, buildMessage(tag, format, args));
    }


    public static void e(String tag, String format, Object... args) {
        if (getLevel() > LEVEL_ERROR) {
            return;
        }
        Log.e(tag, buildMessage(tag, format, args));
    }

    public static void e(String tag, Throwable tr, String format, Object... args) {
        if (getLevel() > LEVEL_ERROR) {
            return;
        }
        Log.e(tag, buildMessage(tag, format, args), tr);
    }


    /**
     * Formats the caller's provided message and prepends useful info like
     * calling thread ID and method name.
     */
    private static String buildMessage(String tag, String format, Object... args) {
        String msg = (args == null) ? format : String.format(Locale.US, format, args);
        StackTraceElement[] trace = new Throwable().fillInStackTrace().getStackTrace();

        String caller = "<unknown>";
        // Walk up the stack looking for the first caller outside of BitWLog.
        // It will be at least two frames up, so start there.
        for (int i = 2; i < trace.length; i++) {
            Class<?> clazz = trace[i].getClass();
            if (!clazz.equals(BlockChainLog.class)) {
                String callingClass = trace[i].getClassName();
                callingClass = callingClass.substring(callingClass.lastIndexOf('.') + 1);
                callingClass = callingClass.substring(callingClass.lastIndexOf('$') + 1);

                caller = callingClass + "." + trace[i].getMethodName();
                break;
            }
        }
        return String.format(Locale.US, "[%d] %s: %s",
                Thread.currentThread().getId(), caller, msg);
    }

}
