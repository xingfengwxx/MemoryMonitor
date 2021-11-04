# JVMTI

- JVM Tools Interface
- 在了解 JVMTI 之前，需要先了解下Java平台调试体系JPDA（Java PlatformDebugger Architecture）。它是Java虚拟机为调试和监控虚拟机专门提供的一套接口。如下图所示，JPDA被抽象为三层实现。其中JVMTI就是JVM对外暴露的接口。JDI是实现了JDWP通信协议的客户端，调试器通过它和JVM中被调试程序通信。

- JVMTI 本质上是在JVM内部的许多事件进行了埋点。通过这些埋点可以给外部提供当前上下文的一些信息。甚至可以接受外部的命令来改变下一步的动作。外部程序一般利用C/C++实现一个JVMTIAgent，在Agent里面注册一些JVM事件的回调。当事件发生时JVMTI调用这些回调方法。Agent可以在回调方法里面实现自己的逻辑。JVMTIAgent是以动态链接库的形式被虚拟机加载的。

![JVMTI](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104170913.png)

- JVMTI的一些重要的功能包括：
  - 重新定义类。
  - 跟踪对象分配和垃圾回收过程。
  - 遵循对象的引用树，遍历堆中的所有对象。
  - 检查 Java 调用堆栈。
  - 暂停（和恢复）所有线程。

# ART TI

- 在 Android 8.0 及更高版本中，ART 工具接口 (ART TI) 可提供某些运行时的内部架构信息，并允许分析器和调试程序影响应用的运行时行为。这可用于实现最先进的性能工具，以便在其他平台上实现原生代理。

- 运行时内部架构信息会提供给已加载到运行时进程中的代理。它们通过直接调用和回调与 ART 通信。

- 提供代理接口 JVMTI 的代码作为运行时插件来实现。

![ART TI](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104171026.png)

## 加载或连接代理

如需在运行时启动时连接代理，请使用以下命令加载 JVMTI 插件和指定的代理：

```
dalvikvm -Xplugin:libopenjdkjvmti.so -agentpath:/path/to/agent/libagent.so …
```

如需将代理连接到已在运行的应用，请使用以下命令：

```
adb shell cmd activity attach-agent [process] /path/to/agent/libagent.so[=agent-options]
```

## API

以下方法已添加到 android.os.Debug 中。

```java
/**
     * Attach a library as a jvmti agent to the current runtime, with the given classloader
     * determining the library search path.
     * Note: agents may only be attached to debuggable apps. Otherwise, this function will
     * throw a SecurityException.
     *
     * @param library the library containing the agent.
     * @param options the options passed to the agent.
     * @param classLoader the classloader determining the library search path.
     *
     * @throws IOException if the agent could not be attached.
     * @throws a SecurityException if the app is not debuggable.
     */
    public static void attachJvmtiAgent(@NonNull String library, @Nullable String options,
            @Nullable ClassLoader classLoader) throws IOException {
```

# 抖音内存监控框架（Kenzo）

![Kenzo](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104171247.png)

# mmap 内存映射

Android中的Binder机制就是mmap来实现的，不仅如此，微信的MMKV key-value组件、美团的 Logan的日志组件 都是基于mmap来实现的。mmap强大的地方在于通过内存映射直接对文件进行读写，减少了对数据的拷贝次数，大大的提高了IO读写的效率。

## Linux文件系统

- 虚拟文件系统层：作用是屏蔽下层具体文件系统操作的差异，为上层的操作提供一个统一的接口。
- 文件系统层 ：具体的文件系统层，一个文件系统一般使用块设备上一个独立的逻辑分区。
- Page Cache （层页高速缓存层）：引入 Cache 层的目的是为了提高 Linux 操作系统对磁盘访问的性能。
- 通用块层：作用是接收上层发出的磁盘请求，并最终发出 I/O 请求。
- I/O 调度层：作用是管理块设备的请求队列。
- 块设备驱动层 ：利用驱动程序，驱动具体的物理块设备。
- 物理块设备层：具体的物理磁盘块。

![](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104171344.png)

## Cache Page与Read/Write操作

由于有了Cache Page的存在，read/write系统调用会有以下的操作，我们拿Read来进行说明：

- 用户进程向内核发起读取文件的请求，这涉及到用户态到内核态的转换。
- 内核读取磁盘文件中的对应数据，并把数据读取到Cache Page中。
- 由于Page Cache处在内核空间，不能被用户进程直接寻址 ，所以需要从Page Cache中拷贝数据到用户进程的堆空间中。

注意，这里涉及到了两次拷贝：第一次拷贝磁盘到Page Cache，第二次拷贝Page Cache到用户内存。最后物理内存的内容是这样的，同一个文件内容存在了两份拷贝，一份是页缓存，一份是用户进程的内存空间。

![Cache Page与Read/Write操作](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104171441.png)

### mmap内存映射原理

mmap是一种内存映射文件的方法，它将一个文件映射到进程的地址空间中，实现文件磁盘地址和进程虚拟地址空间中一段虚拟地址的一一对映关系。实现这样的映射关系后，进程就可以采用指针的方式读写操作这一段内存，而系统会自动回写脏页面到对应的文件磁盘上，即完成了对文件的操作而不必再调用read,write等系统调用函数。相反，内核空间对这段区域的修改也直接反映用户空间，从而可以实现不同进程间的文件共享。

![mmap内存映射原理](https://gitee.com/xingfengwxx/blogImage/raw/master/img/20211104171537.png)

这里我们可以看出mmap系统调用与read/write调用的区别在于：

- mmap只需要一次系统调用（一次拷贝），后续操作不需要系统调用。
- 访问的数据不需要在page cache和用户缓冲区之间拷贝。 

从上所述，当频繁对一个文件进行读取操作时，mmap会比read/write更高效。