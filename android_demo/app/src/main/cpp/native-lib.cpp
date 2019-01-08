#include <jni.h>
#include <string>
#include "PingClient.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_rtttech_rttp_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_rtttech_rttp_MainActivity_startPing(
        JNIEnv *env,
        jobject /* this */,
        jstring server,
        jint rttp_port,
        jint tcp_port
) {
    const char *nativeServer = env->GetStringUTFChars(server, JNI_FALSE);
    startPing(nativeServer, rttp_port, tcp_port);
    env->ReleaseStringUTFChars(server, nativeServer);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_rtttech_rttp_MainActivity_getRttpLatency(
        JNIEnv *env,
        jobject /* this */) {
    return getRttpLatency();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_rtttech_rttp_MainActivity_getTcpLatency(
        JNIEnv *env,
        jobject /* this */) {

    return getTcpLatency();
}

extern "C" JNIEXPORT void JNICALL
Java_com_rtttech_rttp_MainActivity_stopPing(
        JNIEnv *env,
        jobject /* this */) {
    stopPing();
}
