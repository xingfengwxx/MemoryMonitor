package com.wangxingxing.memorymonitor

import android.annotation.SuppressLint
import android.content.Context
import android.os.Build
import android.os.Debug
import android.util.Log
import androidx.annotation.RequiresApi
import com.blankj.utilcode.util.ResourceUtils
import java.io.File
import java.lang.Exception
import java.nio.file.Files
import java.nio.file.Paths
import java.text.SimpleDateFormat
import java.util.*

/**
 * author : 王星星
 * date : 2021/11/3 14:19
 * email : 1099420259@qq.com
 * description :
 */
object Monitor {
    private const val TAG = "wxx"
    private const val LIB_NAME = "memorymonitor"
    private const val ASSETS_LIB_NAME = "libmemorymonitor.so"

    @RequiresApi(Build.VERSION_CODES.O)
    fun init(context: Context) {
        //获取so的地址后加载
        val agentPath = getAgentLibPath(context)
        // 这里是为了演示，如何在程序运行时，动态去加载一个指定的so库
        // 复制其实是没有必要的
        if (agentPath == null) {
            Log.e(TAG, "init: agentPath is null")
            return
        }
        System.load(agentPath)

        //agent.so 连接到 jvmti
        attachAgent(agentPath, context.classLoader)

        // 日志文件的路径
        val logDir = File(context.filesDir, "log")
        if (!logDir.exists())
            logDir.mkdirs()
        val path = "${logDir.absolutePath}/${SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(Date())}.log"
        Log.d(TAG, "init: log file:$path")
        //开启jvmti事件监听
        agentInit(path)
    }

    private fun attachAgent(agentPath: String?, classLoader: ClassLoader) {
        try {
            // 9.0+
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                Debug.attachJvmtiAgent(agentPath!!, null, classLoader)
            } else {
                // 8.0
                val vmDebugClazz = Class.forName("dalvik.system.VMDebug")
                val attachAgentMethod = vmDebugClazz.getMethod("attachAgent", String::class.java)
                attachAgentMethod.isAccessible = true
                attachAgentMethod.invoke(null, agentPath)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    @RequiresApi(Build.VERSION_CODES.O)
    private fun getAgentLibPath(context: Context): String? {
        try {
            val classLoader = context.classLoader
            Log.d(TAG, "getAgentLibPath: classLoader:$classLoader")
            val findLibrary =
                ClassLoader::class.java.getDeclaredMethod("findLibrary", String::class.java)
            //so的地址
            val jvmtiAgentLibPath = findLibrary.invoke(classLoader, LIB_NAME) as String
            Log.d(TAG, "getAgentLibPath: jvmtiAgentLibPath:$jvmtiAgentLibPath")

            //将so拷贝到程序私有目录 /data/data/packageName/files/monitor/agent.so
            val filesDir = context.filesDir
            val jvmtilibDir = File(filesDir, "monitor")
            if (!jvmtilibDir.exists()) {
                jvmtilibDir.mkdirs()
            }
            val agentLibSo = File(jvmtilibDir, "agent.so")
            if (agentLibSo.exists()) {
                agentLibSo.delete()
            }

            if (File(jvmtiAgentLibPath).exists()) {
                Log.d(TAG, "getAgentLibPath: file exist:true")
                Files.copy(
                    Paths.get(File(jvmtiAgentLibPath).absolutePath),
                    Paths.get(agentLibSo.absolutePath)
                )
            } else {
                ResourceUtils.copyFileFromAssets(ASSETS_LIB_NAME, agentLibSo.absolutePath)
            }

            Log.d(TAG, "getAgentLibPath: agentLibSo path:${agentLibSo.absolutePath}")
            return agentLibSo.absolutePath
        } catch (e: Exception) {
            e.printStackTrace()
            Log.e(TAG, "getAgentLibPath: $e")
        }
        return null
    }

    fun release() {
        agentRelease()
    }

    private external fun agentInit(path: String)
    private external fun agentRelease()
}