package com.bitwallet.blockchain.tools.security;

public class PostAuth {
    public static final String TAG = PostAuth.class.getName();

    private static PostAuth instance;

    private PostAuth() {
    }

    public static PostAuth getInstance() {
        if (instance == null) {
            instance = new PostAuth();
        }
        return instance;
    }



}
