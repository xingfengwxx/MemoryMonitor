package com.wangxingxing.memorymonitor

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView
import com.wangxingxing.memorymonitor.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.sampleText.text = stringFromJNI()

        binding.btnCompress.setOnClickListener {
            CompressUtils.qualityCompress(this)
        }
    }

    external fun stringFromJNI(): String

    override fun onDestroy() {
        super.onDestroy()
        Monitor.release()
    }
}