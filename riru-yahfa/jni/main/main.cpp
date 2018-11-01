#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <array>
#include <thread>
#include <vector>
#include <utility>
#include <string>
#include <sys/system_properties.h>

#include "logging.h"

static char package_name[256];
static char app_data_dir[1024];
static int uid;
static int enable_hook;

int is_app_need_hook(JNIEnv *env, jstring appDataDir) {
    if (!appDataDir)
        return 0;

    const char *app_data_dir_ = env->GetStringUTFChars(appDataDir, NULL);
    strcpy(app_data_dir, app_data_dir_);

    int user = 0;
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW("can't parse %s", app_data_dir);
            return 0;
        }
    }
    env->ReleaseStringUTFChars(appDataDir, app_data_dir_);
    
    return 1;
}

int initYahfa(JNIEnv *env){
    LOGI("START INITLIZE YAHFA");
    if(strcmp(package_name, "lab.galaxy.yahfa.demoApp") != 0){
        LOGI("Not target app, skip initlize");
        return 0;
    }

    const char* dexPath = "/sdcard/yahfa.dex";
    const char* pluginPath = "/sdcard/demoPlugin-debug.apk";
    static char dexOptDir[1024];
    strcpy(dexOptDir, app_data_dir);
    strcat(dexOptDir, "files");
    const char* className = "lab.galaxy.yahfa.HookMain";
    const char* methodName = "doHookDefault";

    jclass stringClass, classLoaderClass, dexClassLoaderClass, targetClass;
    jmethodID getSystemClassLoaderMethod, dexClassLoaderContructor, loadClassMethod, targetMethod, targetMethodConstructor;
    jobject systemClassLoaderObject, dexClassLoaderObject, targetMethodObject;
    jstring dexPathString, dexOptDirString, classNameString, tmpString;    
    jobjectArray stringArray;
 
    /* Get SystemClasLoader */
    LOGI("START GET SystemClassLoader");
    //ClassLoader classLoader = getSystemClassLoader();
    stringClass = env->FindClass("java/lang/String");
    classLoaderClass = env->FindClass("java/lang/ClassLoader");
    dexClassLoaderClass = env->FindClass("dalvik/system/DexClassLoader");
    getSystemClassLoaderMethod = env->GetStaticMethodID(classLoaderClass, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    systemClassLoaderObject = env->CallStaticObjectMethod(classLoaderClass, getSystemClassLoaderMethod);
    
    /* Create DexClassLoader */
    LOGI("START CREATE DexClassLoader");
    dexClassLoaderContructor = env->GetMethodID(dexClassLoaderClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    dexPathString = env->NewStringUTF(dexPath);
    dexOptDirString = env->NewStringUTF(dexOptDir);
    dexClassLoaderObject = env->NewObject(dexClassLoaderClass, dexClassLoaderContructor, dexPathString, dexOptDirString, NULL, systemClassLoaderObject);
    
    /* Use DexClassLoader to load class HookMain */
    loadClassMethod = env->GetMethodID(dexClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    classNameString = env->NewStringUTF(className);
    targetClass = (jclass)env->CallObjectMethod(dexClassLoaderObject, loadClassMethod, classNameString);
    
    if (!targetClass) {
        LOGE("Failed to load target class %s", className);
        return -1;
    }
 
    /* Search for HookMain.doHookDefault */
    targetMethod = env->GetStaticMethodID(targetClass, methodName, "(Ljava/lang/ClassLoader;Ljava/lang/ClassLoader;)V");
    if (!targetMethod) {
        LOGE("Failed to load target method %s", methodName);
        return -1;
    }

    /* Create another DexClassLoader */
    LOGI("START CREATE another DexClassLoader");
    //release memory
    //获得成就：tombstone * 1
    //env->ReleaseStringUTFChars(dexPathString, dexPath);
    //env->DeleteLocalRef(dexClassLoaderObject);
    //create ojbext
    dexClassLoaderContructor = env->GetMethodID(dexClassLoaderClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    dexPathString = env->NewStringUTF(pluginPath);
    dexClassLoaderObject = env->NewObject(dexClassLoaderClass, dexClassLoaderContructor, dexPathString, dexOptDirString, NULL, systemClassLoaderObject);

    /* Invoke target method */
    env->CallStaticVoidMethod(targetClass, targetMethod, dexClassLoaderObject, systemClassLoaderObject);
    LOGD("ALL DONE!");

    return 1;
}


extern "C" {
__attribute__((visibility("default")))
void nativeForkAndSpecializePre(JNIEnv *env, jclass clazz, jint _uid, jint gid, jintArray gids,
                                jint runtime_flags, jobjectArray rlimits, jint mount_external,
                                jstring se_info, jstring se_name, jintArray fdsToClose,
                                jintArray fdsToIgnore,
                                jboolean is_child_zygote, jstring instructionSet,
                                jstring appDataDir) {
    uid = _uid;
    enable_hook = is_app_need_hook(env, appDataDir);
}

__attribute__((visibility("default")))
int nativeForkAndSpecializePost(JNIEnv *env, jclass clazz, jint res) {
    if(enable_hook){
        if(res != 0){
            LOGI("Not forked process, skipping...");
            return 1;
        }
        LOGD("Start hook on %s.", package_name);
        initYahfa(env);
    }else{
        LOGD("APP %s do not need hook.", package_name);
    }
    return !enable_hook;
}
}