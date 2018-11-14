package com.forrest.tool.privkeytool

import android.app.Activity
import android.content.ClipboardManager
import android.content.Context
import android.os.Bundle
import android.preference.PreferenceManager
import android.support.v7.app.AppCompatActivity
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.view.inputmethod.InputMethodManager
import android.widget.AdapterView
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import com.bitwallet.blockchain.eth.EthManger
import com.bitwallet.blockchain.model.CoinType
import com.bitwallet.blockchain.tools.TypesConverter
import com.jniwrappers.BitWallet
import com.jniwrappers.MnemonicCode
import kotlinx.android.synthetic.main.activity_main.*
import kotlinx.android.synthetic.main.content_main.*
import java.util.ArrayList

class MainActivity : AppCompatActivity() {

    init {
        System.loadLibrary("bitw-core-lib")
    }

    private var mPhrase: String? = null
    private var mCurrency: String? = null
    private val test = true
    private val words: ArrayList<EditText> by lazy { arrayListOf(word1, word2, word3, word4, word5, word6, word7, word8,word9, word10, word11, word12) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        setSupportActionBar(toolbar)


        importBtn.setOnClickListener { importPhrase() }
        currencySpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onNothingSelected(parent: AdapterView<*>?) {
            }

            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                mCurrency = parent?.getItemAtPosition(position).toString()
                getPrivKeyAndAddr()
            }
        }

        privkey_tv.setOnLongClickListener { v -> copyToClipboard(v) }
        addr_tv.setOnLongClickListener { v -> copyToClipboard(v) }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        // Inflate the menu; this adds items to the action bar if it is present.
        menuInflater.inflate(R.menu.menu_main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_history -> { importLastPhrase(); return true}
            else -> super.onOptionsItemSelected(item)
        }
    }

    private fun importLastPhrase() {
        if (test) mPhrase = "fine invite youth water sudden language weasel never law series jacket congress"
        else mPhrase = getLastPhrase()

        val phrases = mPhrase!!.split(" ")

        phrases.forEachIndexed { index, s ->  words[index].setText(s)}
    }

    private fun importPhrase() {
        getPhrase()?.apply {
            if (MnemonicCode.isPaperKeyValid(baseContext, this)) {
                mPhrase = this
                savePhrase()
                getPrivKeyAndAddr()
                hideKeyboard()

//                removeEditTextFocus()
//                currencySpinner.requestFocus()
                toast("助记词输入正确！")
                return
            }
        }

        toast("助记词输入有误，请重新输入！")
    }

//    fun removeEditTextFocus(){
//        val v = currentFocus
//        if(v is EditText) v.clearFocus()
//    }

    private fun getPhrase(): String? {


        val sb = StringBuilder()
        words.forEach {
            val w = it.text.toString().toLowerCase().trim()
            if (w.isEmpty()) return null
            sb.append("$w ")
        }

        return sb.substring(0, sb.lastIndex)
    }

    fun savePhrase() = PreferenceManager.getDefaultSharedPreferences(this).edit().putString("lst_phrase", mPhrase).commit()

    fun getLastPhrase() = PreferenceManager.getDefaultSharedPreferences(this).getString("lst_phrase", "")


    fun getPrivKeyAndAddr() {
        mPhrase ?: return
        mCurrency ?: return

        val nullTermPhrase: ByteArray = TypesConverter.getNullTerminatedPhrase(mPhrase!!.toByteArray())
        val type = when (mCurrency) {
            "BTC" -> CoinType.BTC
            "BCH" -> CoinType.BCH
            "BTN" -> CoinType.BTN
            "QTUM" -> CoinType.QTUM
            "ETH" -> CoinType.ETH
            "EOS" -> CoinType.EOS
            else -> CoinType.BTC
        }


        val privkey = BitWallet.getStrFirstPrivKey(nullTermPhrase, type.bip44Index)
        var addr : String
        if (type == CoinType.EOS) {
            addr = EthManger.getAddress(nullTermPhrase)
        } else {
            val pubKey = MnemonicCode.getMasterPubKey(nullTermPhrase, type)
            addr = BitWallet.getFirstAddress(pubKey, type.bip44Index)
        }


        Log.d("privkeytool", type.coinCode + " privkey:$privkey   addr:$addr")

        privkey_tv.text = privkey
        addr_tv.text = addr
    }

    private fun toast(msg: String) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }


    //复制到粘贴板
    fun copyToClipboard(v: View): Boolean {
        if (v is TextView) {
            val content = v.text
            val cmb = this.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
            cmb.text = content

            toast("复制成功")
        }

        return true
    }

    fun hideKeyboard() {
        (currentFocus ?: window?.decorView)?.apply {
            if (this != null) {
                val imm = this.context.getSystemService(Context.INPUT_METHOD_SERVICE) as InputMethodManager
                imm.hideSoftInputFromWindow(this.windowToken, 0)
            }
        }
    }

}
