#include <jni.h>
#include <android_native_app_glue.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "include/org_p4a_minimal_PythonService.h"

extern "C"
{
    #include "utils.c"
}


JNIEXPORT jint JNICALL Java_org_p4a_minimal_PythonService_nativeMain(JNIEnv *env, jobject thiz){
    LOGI("Starting minimal bootstrap.");
    //g_state = state;
    jobject state = thiz;
    jclass clazz = env->GetObjectClass(thiz);

    //char *env_argument = NULL;
    //int fd = -1;

    const char* str;
    jboolean isCopy;

    const char* internal_data_path;
    const char* external_data_path;

    LOGI("Initialize Python for Android");
    //env_argument = getenv("ANDROID_ARGUMENT");
    //setenv("ANDROID_APP_PATH", env_argument, 1);
    //setenv("PYTHONVERBOSE", "2", 1);
    Py_SetProgramName("python-android");
    Py_Initialize();
    //PySys_SetArgv(argc, argv);

    // ensure threads will work.
    PyEval_InitThreads();

    // our logging module for android
    initandroidembed();

    // get the APK filename, and set it to ANDROID_APK_FN
    //ANativeActivity* activity = state->activity;

    //(*activity->vm)->AttachCurrentThread(activity->vm, &env, 0);
    //jclass clazz = (*env)->GetObjectClass(env, activity->clazz);
    jmethodID methodID = env->GetMethodID(clazz, "getPackageCodePath", "()Ljava/lang/String;");
    jobject result = env->CallObjectMethod(clazz, methodID);

    str = env->GetStringUTFChars((jstring)result, &isCopy);
    LOGI("Looked up package code path: %s", str);
    setenv("ANDROID_APK_FN", str, 1);

    methodID = env->GetMethodID(clazz, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
    jobject appInfo = env->CallObjectMethod(clazz, methodID);
    jfieldID fieldID = env->GetFieldID( env->GetObjectClass(appInfo), "nativeLibraryDir", "Ljava/lang/String;");
    result = env->GetObjectField(appInfo, fieldID);

    str = env->GetStringUTFChars((jstring)result, &isCopy);
    LOGI("Looked up library code path: %s", str);
    setenv("ANDROID_LIB_PATH", str, 1);

    //(*activity->vm)->DetachCurrentThread(activity->vm);

    // From: http://stackoverflow.com/a/18409813/798575
    //  and: http://stackoverflow.com/q/10166638/798575
    jobject javaObject = NULL;
    jclass javaClass = NULL;
    jmethodID javaMethodID = NULL;
    jobject pathObject = NULL;

    jmethodID getFilesDir = env->GetMethodID(clazz, "getFilesDir", "()Ljava/io/File;");
    javaObject = env->CallObjectMethod(clazz, getFilesDir);
    javaClass = env->GetObjectClass(javaObject);
    javaMethodID = env->GetMethodID(javaClass, "getAbsolutePath", "()Ljava/lang/String;");
    pathObject = env->CallObjectMethod(javaObject, javaMethodID);
    internal_data_path = env->GetStringUTFChars((jstring)pathObject, NULL);

    env->DeleteLocalRef(pathObject);
    env->DeleteLocalRef(javaClass);
    env->DeleteLocalRef(javaObject);
    //jni->DeleteLocalRef(clazz);

    jmethodID getExternalFilesDir = env->GetMethodID(clazz, "getExternalFilesDir", "()Ljava/io/File;");
    javaObject = env->CallObjectMethod(clazz, getExternalFilesDir);
    javaClass = env->GetObjectClass(javaObject);
    javaMethodID = env->GetMethodID(javaClass, "getAbsolutePath", "()Ljava/lang/String;");
    pathObject = env->CallObjectMethod(javaObject, javaMethodID);
    external_data_path = env->GetStringUTFChars((jstring)pathObject, NULL);

    env->DeleteLocalRef(pathObject);
    env->DeleteLocalRef(javaClass);
    env->DeleteLocalRef(javaObject);
    //jni->DeleteLocalRef(clazz);

    // set some envs
    setenv("ANDROID_INTERNAL_DATA_PATH", internal_data_path, 1);
    setenv("ANDROID_EXTERNAL_DATA_PATH", external_data_path, 1);
    LOGI("Internal data path is: %s", internal_data_path);
    LOGI("External data path is: %s", external_data_path);

    // extract the Makefile, needed for sysconfig
    // JAVA: this.getResources().getAssets()
    jobject assetManager = NULL;

    jmethodID getResources = env->GetMethodID(clazz, "getResources", NULL);
    javaObject = env->CallObjectMethod(clazz, getResources);
    javaClass = env->GetObjectClass(javaObject);
    javaMethodID = env->GetMethodID(javaClass, "getAssets", NULL);
    assetManager = env->CallObjectMethod(javaObject, javaMethodID);

    env->DeleteLocalRef(javaClass);
    env->DeleteLocalRef(javaObject);

    AAssetManager *am = AAssetManager_fromJava(env, assetManager);

    char bootstrap_fn[512];
    snprintf(bootstrap_fn, 512, "%s/_bootstrap.py", internal_data_path);
    if (asset_extract(am, "_bootstrap.py", bootstrap_fn) < 0) {
        LOGW("Unable to extract _bootstrap.py");
        return 0;
    }

    // pass a module name as argument: _bootstrap.py use it as the main module
    int argc;
    char * argv[2];
    argc = 2;
    argv[0] = "_bootstrap.py";
    argv[1] = "service";

    PySys_SetArgv(argc, argv);

    // run the python bootstrap
    LOGI("Run _bootstrap.py >>>");
    FILE *fhd = fopen(bootstrap_fn, "rb");
    if (fhd == NULL) {
        LOGW("Cannot open _bootstrap.py (errno=%d:%s)", errno, strerror(errno));
        return 0;
    }
    int ret = PyRun_SimpleFile(fhd, bootstrap_fn);
    fclose(fhd);
    LOGI("Run _bootstrap.py (ret=%d) <<<", ret);

    if (PyErr_Occurred() != NULL) {
        PyErr_Print();
        if (Py_FlushLine())
            PyErr_Clear();
    }

    Py_Finalize();
    LOGI("Python for android ended.");
    return 0;
}

//
//jint JNI_OnLoad(JavaVM* vm, void* reserved)
//{
//    JNIEnv* env;
//    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
//        return -1;
//    }
//
//
//    // Get jclass with env->FindClass.
//    //jclass class = env->FindClass(env,"ClassName")
//    // Register methods with env->RegisterNatives.
//
//
//    return JNI_VERSION_1_6;
//}



void android_main(struct android_app* state) {
    app_dummy();
}
