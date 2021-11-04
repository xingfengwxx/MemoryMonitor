package com.wangxingxing.memorymonitor

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import java.io.ByteArrayOutputStream
import java.io.File
import java.io.FileOutputStream

object CompressUtils {

    fun qualityCompress(applicationContext: Context) {
        Log.d("ning","质量压缩")
        val bitmap = BitmapFactory.decodeResource(applicationContext.resources, R.drawable.photo1)
        try {
            val baos = ByteArrayOutputStream()
            val quality = 50
            bitmap.compress(Bitmap.CompressFormat.JPEG, quality, baos);

            val fos = FileOutputStream(File(applicationContext.filesDir, "${System.currentTimeMillis()}.JPG"))
            fos.write(baos.toByteArray())
            fos.flush()
            fos.close()
        } catch (ex: Throwable) {
            ex.printStackTrace()
        }
    }

}