#include <jni.h>
#include <android_native_app_glue.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "include/org_p4a_minimal_PythonService.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "python service", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "python service", __VA_ARGS__))

extern "C" {
    #include "utils.c"
}


JNIEXPORT jint JNICALL Java_org_p4a_minimal_PythonService_nativeServiceStart(JNIEnv *env, jclass clazz,
                                                                     jstring j_getPackageCodePath, // this.getPackageCodePath()
                                                                     jstring j_nativeLibraryDir, // this.getApplicationInfo().nativeLibraryDir
                                                                     jstring j_getFilesDir, // this.getFilesDir().getAbsolutePath()
                                                                     jstring j_getExternalFilesDir, // getExternalFilesDir().getAbsolutePath()
                                                                     jobject j_assetManager) // getAssets()
{
    app_dummy();

    LOGI("Starting minimal service bootstrap.");

    jboolean iscopy;
    const char *android_apk_fn = env->GetStringUTFChars(j_getPackageCodePath, &iscopy);
    const char *android_lib_path = env->GetStringUTFChars(j_nativeLibraryDir, &iscopy);
    const char *android_internaldata_path = env->GetStringUTFChars(j_getFilesDir, &iscopy);
    const char *android_externaldata_path = env->GetStringUTFChars(j_getExternalFilesDir, &iscopy);
    AAssetManager *am = AAssetManager_fromJava(env, j_assetManager);

    LOGI("Initialize Python for Android");

    Py_SetProgramName("python-android");
    Py_Initialize();

    // ensure threads will work.
    PyEval_InitThreads();

    // our logging module for android
    initandroidembed();

    // get the APK filename, and set it to ANDROID_APK_FN
    // ??? (*activity->vm)->AttachCurrentThread(activity->vm, &env, 0);

    LOGI("Looked up package code path: %s", android_apk_fn);
    setenv("ANDROID_APK_FN", android_apk_fn, 1);

    LOGI("Looked up library code path: %s", android_lib_path);
    setenv("ANDROID_LIB_PATH", android_lib_path, 1);

    // ??? (*activity->vm)->DetachCurrentThread(activity->vm);

    // set some envs
    LOGI("Internal data path is: %s", android_internaldata_path);
    setenv("ANDROID_INTERNAL_DATA_PATH", android_internaldata_path, 1); // internalDataPath

    LOGI("External data path is: %s", android_externaldata_path);
    setenv("ANDROID_EXTERNAL_DATA_PATH", android_externaldata_path, 1); // externalDataPath


    char bootstrap_fn[512];
    snprintf(bootstrap_fn, 512, "%s/_bootstrap.py", android_internaldata_path);
    if (asset_extract(am, "_bootstrap.py", bootstrap_fn) < 0) {
        LOGW("Unable to extract _bootstrap.py");
        return -1;
    }

    // pass a module name as argument: _bootstrap.py use it as the main module
    int argc;
    char * argv[2];
    argc = 2;
    argv[0] = "_bootstrap.py";
    argv[1] = "service/main";

    PySys_SetArgv(argc, argv);

    // run the python bootstrap
    LOGI("Run _bootstrap.py >>>");
    FILE *fhd = fopen(bootstrap_fn, "rb");
    if (fhd == NULL) {
        LOGW("Cannot open _bootstrap.py (errno=%d:%s)", errno, strerror(errno));
        return -1;
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


void android_main(struct android_app* state) {
    app_dummy();
}
