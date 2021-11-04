package com.wangxingxing.memorymonitor

import android.app.Application
import com.blankj.utilcode.util.Utils

/**
 * author : 王星星
 * date : 2021/11/4 10:44
 * email : 1099420259@qq.com
 * description :
 */
class App : Application() {

    override fun onCreate() {
        super.onCreate()

        Utils.init(this)
        Monitor.init(this)
    }
}