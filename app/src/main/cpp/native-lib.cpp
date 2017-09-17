#include <jni.h>
#include <android/log.h>
#include <stdexcept>
#include <string>

using namespace std;

#include "SoundTouch.h"
#include "SoundStretch/WavFile.h"
#include <pthread.h>
#define LOGV(...)   __android_log_print((int)ANDROID_LOG_INFO, "SOUNDTOUCH", __VA_ARGS__)
//#define LOGV(...)


//用于保存可能出现的c ++异常错误消息的字符串。 请注意，这不是
//线程安全，但预期异常是不会的特殊情况
//并行发生在几个线程中。
static string _errMsg = "";


#define DLL_PUBLIC __attribute__ ((visibility ("default")))
#define BUFF_SIZE 4096


using namespace soundtouch;


// Set error message to return
static void _setErrmsg(const char *msg)
{
    _errMsg = msg;
}


#ifdef _OPENMP
#include <pthread.h>
extern pthread_key_t gomp_tls_key;
static void * _p_gomp_tls = NULL;

///初始化OpenMP线程的功能。
///
///这是关于OpenMP的Android NDK v10中的错误的解决方法：OpenMP只有在
///从Android App主线程中调用，因为在主线程中gomp_tls的存储是
///正确设置，但是，Android没有正确地初始化其他线程的gomp_tls存储。
///因此，如果从主线程的其他线程调用OpenMP例程，
///由于未初始化存储上的NULL指针访问，OpenMP例程将使应用程序崩溃。
///
///此解决方法从主线程存储gomp_tls存储，并将副本存储到其他线程。
///为了使其工作，应用程序主线程至少要调用“getVersionString”
///例程。
static int _init_threading(bool warn)
{
	void *ptr = pthread_getspecific(gomp_tls_key);
	LOGV("JNI thread-specific TLS storage %ld", (long)ptr);
	if (ptr == NULL)
	{
		LOGV("JNI set missing TLS storage to %ld", (long)_p_gomp_tls);
		pthread_setspecific(gomp_tls_key, _p_gomp_tls);
	}
	else
	{
		LOGV("JNI store this TLS storage");
		_p_gomp_tls = ptr;
	}
	// Where critical, show warning if storage still not properly initialized
	if ((warn) && (_p_gomp_tls == NULL))
	{
		_setErrmsg("Error - OpenMP threading not properly initialized: Call SoundTouch.getVersionString() from the App main thread!");
		return -1;
	}
	return 0;
}

#else
static int _init_threading(bool warn)
{
    // do nothing if not OpenMP build
    return 0;
}


JNIEXPORT jstring JNICALL
Java_com_example_fmy_myapplication_SoundTouch_getVersionString(JNIEnv *env, jclass type) {
    const char *verStr;

    LOGV("JNI call SoundTouch.getVersionString");

    // 调用示例SoundTouch程序
    verStr = SoundTouch::getVersionString();

    /// gomp_tls storage bug workaround - see comments in _init_threading() function!
    _init_threading(false);

    int threads = 0;
#pragma omp parallel
    {
#pragma omp atomic
        threads ++;
    }
    LOGV("JNI thread count %d", threads);

    // 用String 作为版本返回
    return env->NewStringUTF(verStr);
}


#endif


extern pthread_key_t gomp_tls_key;
static void * _p_gomp_tls = NULL;


extern  "C"
JNIEXPORT void JNICALL
Java_com_example_fmy_myapplication_SoundTouch_deleteInstance(JNIEnv *env, jobject instance,
                                                             jlong handle) {

    // TODO
    SoundTouch *ptr = (SoundTouch*)handle;
    delete ptr;
}

extern  "C"
JNIEXPORT jstring JNICALL
Java_com_example_fmy_myapplication_SoundTouch_getErrorString(JNIEnv *env, jclass type) {

    jstring result = env->NewStringUTF(_errMsg.c_str());
    _errMsg.clear();

    return result;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_fmy_myapplication_SoundTouch_setPitchSemiTones(JNIEnv *env, jobject instance,
                                                                jlong handle, jfloat pitch) {
    SoundTouch *ptr = (SoundTouch*)handle;
    ptr->setPitchSemiTones(pitch);
    // TODO

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_fmy_myapplication_SoundTouch_setTempo(JNIEnv *env, jobject instance,
                                                           jlong handle, jfloat tempo) {

    // TODO
    SoundTouch *ptr = (SoundTouch*)handle;
   ptr->setTempo(tempo);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_fmy_myapplication_SoundTouch_setSpeed(JNIEnv *env, jobject instance,
                                                           jlong handle, jfloat speed) {

    // TODO
    SoundTouch *ptr = (SoundTouch*)handle;
    ptr->setRate(speed);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_example_fmy_myapplication_SoundTouch_newInstance(JNIEnv *env, jclass type) {


    return (jlong)(new  SoundTouch());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_fmy_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
// Processes the sound file
static void _processFile(SoundTouch *pSoundTouch, const char *inFileName, const char *outFileName)
{
    int nSamples;
    int nChannels;
    int buffSizeSamples;
    SAMPLETYPE sampleBuffer[BUFF_SIZE];

    // 打开输入文件
    WavInFile inFile(inFileName);
    int sampleRate = inFile.getSampleRate();
    int bits = inFile.getNumBits();
    nChannels = inFile.getNumChannels();

    //创建输出文件
    WavOutFile outFile(outFileName, sampleRate, bits, nChannels);

    pSoundTouch->setSampleRate(sampleRate);
    pSoundTouch->setChannels(nChannels);

    assert(nChannels > 0);
    buffSizeSamples = BUFF_SIZE / nChannels;

    // Process samples read from the input file
    //读取输入的文件进行样本处理
    while (inFile.eof() == 0)
    {
        int num;

        // Read a chunk of samples from the input file
        //从输入文件中读取一大块样本
        num = inFile.read(sampleBuffer, BUFF_SIZE);
        nSamples = num / nChannels;

        // 将样品放入SoundTouch处理器
        pSoundTouch->putSamples(sampleBuffer, nSamples);

        // 从SoundTouch处理器读取已准备好的样本，并写入输出文件
        //注意：
         // - 'receiveSamples'不一定返回任何样本
         //在一些回合！
         // - 另一方面，在一些“receiveSamples”中可能会有更多的内容
         //准备样本比适合“sampleBuffer”，因此
         //'receiveSamples'调用被迭代了很多次
         //输出样品。
        do
        {
            nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
            outFile.write(sampleBuffer, nSamples * nChannels);
        } while (nSamples != 0);
    }

    //现在，输入文件被处理，但'flush'几个最后的样本是
    //隐藏在SoundTouch的内部处理管道中。
    pSoundTouch->flush();
    do
    {
        nSamples = pSoundTouch->receiveSamples(sampleBuffer, buffSizeSamples);
        outFile.write(sampleBuffer, nSamples * nChannels);
    } while (nSamples != 0);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_fmy_myapplication_SoundTouch_processFile(
        JNIEnv *env, jobject instance, jlong handle, jstring jinputFile, jstring joutputFile) {
    SoundTouch *ptr = (SoundTouch*)handle;

    const char *inputFile = env->GetStringUTFChars(jinputFile, 0);
    const char *outputFile = env->GetStringUTFChars(joutputFile, 0);

    LOGV("JNI process file %s", inputFile);

    /// gomp_tls storage bug workaround - see comments in _init_threading() function!
    if (_init_threading(true)) return -1;

    try
    {
        _processFile(ptr, inputFile, outputFile);
    }
    catch (const runtime_error &e)
    {
        const char *err = e.what();
        // An exception occurred during processing, return the error message
        //在处理过程中出现异常，返回错误消息
        LOGV("JNI exception in SoundTouch::processFile: %s", err);
        _setErrmsg(err);
        return -1;
    }


    env->ReleaseStringUTFChars(jinputFile, inputFile);
    env->ReleaseStringUTFChars(joutputFile, outputFile);

    return 0;
}