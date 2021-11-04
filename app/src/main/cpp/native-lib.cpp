#include <jni.h>
#include <string>
#include <android/log.h>
#include "jvmti.h"
#include "MemoryFile.h"

#define LOG_TAG "wxx"
#define ALOGE(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,format, __VA_ARGS__)

jvmtiEnv *mJvmtiEnv = NULL;
jlong tag = 0;
MemoryFile *memoryFile;

//初始化工作
extern "C"
JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM *vm, char *options, void *reserved) {
    //准备jvmti环境
    vm->GetEnv(reinterpret_cast<void **>(&mJvmtiEnv), JVMTI_VERSION_1_2);

    //开启jvmti的能力
    jvmtiCapabilities caps;
    //获取所有的能力
    mJvmtiEnv->GetPotentialCapabilities(&caps);
    mJvmtiEnv->AddCapabilities(&caps);
    return JNI_OK;
}

//调用System.Load()后会回调该方法
extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

void JNICALL objectAlloc(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread,
                         jobject object, jclass object_klass, jlong size) {
    //对象创建
    //给对象打tag，后续在objectFree()内可以通过该tag来判断是否成对出现释放
    tag += 1;
    jvmti_env->SetTag(object, tag);
    //获取线程信息
    jvmtiThreadInfo threadInfo;
    jvmti_env->GetThreadInfo(thread, &threadInfo);
    //获得 创建的对象的类签名
    char *classSignature;
    jvmti_env->GetClassSignature(object_klass, &classSignature, nullptr);
    //获得堆栈信息
    //char *stackInfo = createStackInfo(jvmti_env, jni_env, thread, 10);
    if (size > 10000) ALOGE("%s", "Memory warning");
    //ALOGE("object alloc, Thread is %s, class is %s, size is %lld, tag is %lld", threadInfo.name,classSignature, size, tag);

    // 写入日志文件
    char str[500];
    char *format = "object alloc, Thread is %s, class is %s, size is %lld, tag is %lld \r\n";
    sprintf(str, format, threadInfo.name, classSignature, size, tag);
    memoryFile->write(str, sizeof(char) * strlen(str));

    jvmti_env->Deallocate((unsigned char *) classSignature);
}

void JNICALL objectFree(jvmtiEnv *jvmti_env, jlong tag) {
    //对象释放
    // 写入日志文件
    char str[50];
    char *format = "objectFree tag is %lld \r\n";
    sprintf(str, format, tag);
    memoryFile->write(str, sizeof(char) * strlen(str));
}

void JNICALL methodEntry(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method) {
    //方法进入
    jclass clazz;
    char *signature;
    char *methodName;
    //获得方法对应的类
    jvmti_env->GetMethodDeclaringClass(method, &clazz);
    //获得类的签名
    jvmti_env->GetClassSignature(clazz, &signature, 0);
    //获得方法名字
    jvmti_env->GetMethodName(method, &methodName, NULL, NULL);
    //ALOGE("methodEntry method name is %s %s", signature, methodName);

    // 写入日志文件
//    char str[500];
//    char *format = "methodEntry method name is %s %s \r\n";
//    sprintf(str, format, signature, methodName);
//    memoryFile->write(str, sizeof(char) * strlen(str));

    jvmti_env->Deallocate((unsigned char *) methodName);
    jvmti_env->Deallocate((unsigned char *) signature);
}

void JNICALL methodExit(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method,
                        jboolean was_popped_by_exception,
                        jvalue return_value) {
    //方法退出
}

void JNICALL garbageCollectionStart(jvmtiEnv *jvmti_env) {
    // GC 开始回收
    // 写入日志文件
    char *str = "GarbageCollectionStart \r\n";
    memoryFile->write(str, sizeof(char) * strlen(str));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_wangxingxing_memorymonitor_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++ MemoryMonitor";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wangxingxing_memorymonitor_Monitor_agentInit(JNIEnv *env, jobject thiz, jstring _path) {
    // 日志文件路径
    // 当从JNI函数GetStringUTFChars函数中返回得到字符串B时，
    // 如果B是原始字符串java.lang.String的一份拷贝，则isCopy 被赋值为JNI_TRUE。
    // 如果B是和原始字符串指向的是JVM中的同一份数据，则isCopy 被赋值为JNI_FALSE。
    // 当isCopy 为JNI_FALSE时，本地代码绝不能修改字符串的内容，否则JVM中的原始字符串也会被修改，这会打破Java语言中字符串不可变的规则。
    // 通常，我们不必关心JVM是否会返回原始字符串的拷贝，只需要为isCopy传递NULL作为参数。
    const char *path = env->GetStringUTFChars(_path, NULL);
    memoryFile = new MemoryFile(path);

    //开启jvm事件监听
    jvmtiEventCallbacks callbacks;
    //memset(&callbacks, 0, sizeof(callbacks));
    callbacks.MethodEntry = &methodEntry;
    //callbacks.MethodExit = &methodExit;
    callbacks.VMObjectAlloc = &objectAlloc;
    callbacks.ObjectFree = &objectFree;
    callbacks.GarbageCollectionStart = &garbageCollectionStart;
    //设置回调函数
    mJvmtiEnv->SetEventCallbacks(&callbacks, sizeof(callbacks));
    //开启监听
    mJvmtiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_OBJECT_FREE, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    //mJvmtiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);

    env->ReleaseStringUTFChars(_path, path);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_wangxingxing_memorymonitor_Monitor_agentRelease(JNIEnv *env, jobject thiz) {
    delete memoryFile;
    mJvmtiEnv->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_VM_OBJECT_ALLOC, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_OBJECT_FREE, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_METHOD_EXIT, NULL);
    mJvmtiEnv->SetEventNotificationMode(JVMTI_DISABLE, JVMTI_EVENT_GARBAGE_COLLECTION_START, NULL);
}