package com.bitwallet.blockchain.tools;

import android.content.Context;
import android.content.res.AssetManager;

import com.bitwallet.blockchain.tools.manger.BitWReportsManager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.Normalizer;
import java.util.ArrayList;
import java.util.List;



public class Bip39Reader {

    private static final String TAG = Bip39Reader.class.getName();
    public static final int WORD_LIST_SIZE = 2048;

    /**
     * BIP39
     */
    public static String[] LANGS_BIP39 = {"en"};

    //if lang is null then all the lists
    public static List<String> bip39List(Context context, String lang) {

        String[] langs = null;
        if (lang == null)
            langs = LANGS_BIP39; //return all the words for all langs
        else {
            boolean exists = false;
            for (String s :LANGS_BIP39) if (s.equalsIgnoreCase(lang)) exists = true;
            if (exists)
                langs = new String[]{lang};//if lang is one of the language we support for paper key creation, then use it
            else
                langs = new String[]{"en"};// if not than return 'en'
        }

        List<String> result = new ArrayList<>();

        for (String l : langs) {
            String fileName = "words/" + l + "-BIP39Words.txt";
            List<String> wordList = new ArrayList<>();
            BufferedReader reader = null;
            try {
                AssetManager assetManager = context.getResources().getAssets();
                InputStream inputStream = assetManager.open(fileName);
                reader = new BufferedReader(new InputStreamReader(inputStream));
                String line;

                while ((line = reader.readLine()) != null) {
                    wordList.add(cleanWord(line));
                }
            } catch (Exception ex) {
                ex.printStackTrace();

            } finally {
                try {
                    if (reader != null)
                        reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (wordList.size() % WORD_LIST_SIZE != 0) {
                BitWReportsManager.reportBug(new IllegalArgumentException("The list size should divide by " + WORD_LIST_SIZE), true);
            }
            result.addAll(wordList);

        }
        return result;
    }

    public static String cleanWord(String word) {
        String w = Normalizer.normalize(word.trim().replace("　", "")
                .replace(" ", ""), Normalizer.Form.NFKD);
        return w;
    }
}
