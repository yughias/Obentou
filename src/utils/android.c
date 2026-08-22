#include "android.h"

#ifdef __ANDROID__

#include <jni.h>
#include <string.h>
#include <math.h>

#include "minIni.h"

#include "core.h"
#include "utils/archive.h"
#include "utils/argument.h"

static bool emu_rewind;

bool android_is_rewind() {
    return emu_rewind;
}

static const char* detect_from_rom(const archive_t* rom_archive){
    for(int i = 0; i < n_cores; i++){
        if(cores[i].detect(rom_archive, NULL)){
            return cores[i].name;
        }
    }

    return "Unknown";
}

JNIEXPORT void JNICALL
Java_emu_MainActivity_adjustSpeed(JNIEnv *env, jobject thiz, jint jspeed_idx) {
    int requested_speed = (int)jspeed_idx;

    bool is_paused = core_is_paused(&emu_ctx);
    emu_rewind = false;

    switch (requested_speed) {
        case -1:
            if (is_paused)
                core_switch_pause(&emu_ctx);
            emu_rewind ^= 1;
        break;

        case 0:
        {
            if (!is_paused)
                core_switch_pause(&emu_ctx);
        }
        break;

        case 1:
        case 2:
        case 4:
        case 8:
        {
            if (is_paused)
                core_switch_pause(&emu_ctx);
            ctx_args_t arg = { &emu_ctx, log2f(requested_speed) + 0.5f };
            core_ctx_set_speed(&arg);
            if (core_is_paused(&emu_ctx))
                core_switch_pause(&emu_ctx);
        }
        break;
    }
}


JNIEXPORT jstring JNICALL
Java_launcher_LauncherActivity_getSystemName(JNIEnv *env, jobject thiz, jstring jrom_path) {
    const char *rom_path = (*env)->GetStringUTFChars(env, jrom_path, 0);
    
    if (!rom_path) {
        return NULL;
    }

    archive_t* rom_archive = archive_load(rom_path);
    const char* name  = detect_from_rom(rom_archive);
    archive_free(rom_archive);

    (*env)->ReleaseStringUTFChars(env, jrom_path, rom_path);

    return (*env)->NewStringUTF(env, name);
}

JNIEXPORT jobjectArray JNICALL
Java_launcher_BiosActivity_getSupportedSystems(JNIEnv *env, jobject thiz) {
    int count = 0;
    for (int i = 0; i < n_cores; i++) {
        count += cores[i].has_bios;
    }

    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, count, stringClass, NULL);

    int idx = 0;
    for (int i = 0; i < n_cores; i++) {
        if (cores[i].has_bios) {
            jstring jstr = (*env)->NewStringUTF(env, cores[i].name);
            (*env)->SetObjectArrayElement(env, result, idx++, jstr);
            (*env)->DeleteLocalRef(env, jstr);
        }
    }

    return result;
}

JNIEXPORT jstring JNICALL
Java_launcher_BiosActivity_getDefaultBiosPath(JNIEnv *env, jobject thiz, jstring config_ini_path, jstring system_name) {
    const char *c_config_path = (*env)->GetStringUTFChars(env, config_ini_path, NULL);
    const char *c_system_name = (*env)->GetStringUTFChars(env, system_name, NULL);

    char bios_path[1024] = {0};

    ini_gets(c_system_name, "BIOS", "", bios_path, sizeof(bios_path), c_config_path);

    (*env)->ReleaseStringUTFChars(env, config_ini_path, c_config_path);
    (*env)->ReleaseStringUTFChars(env, system_name, c_system_name);

    return (*env)->NewStringUTF(env, bios_path);
}

JNIEXPORT void JNICALL
Java_launcher_BiosActivity_setSystemBios(JNIEnv *env, jobject thiz, jstring config_ini_path, jstring bios_path, jstring system_name) {
    const char *c_config_path = (*env)->GetStringUTFChars(env, config_ini_path, NULL);
    const char *c_bios_path = (*env)->GetStringUTFChars(env, bios_path, NULL);
    const char *c_system_name = (*env)->GetStringUTFChars(env, system_name, NULL);

    ini_puts(c_system_name, "BIOS", c_bios_path, c_config_path);

    (*env)->ReleaseStringUTFChars(env, config_ini_path, c_config_path);
    (*env)->ReleaseStringUTFChars(env, bios_path, c_bios_path);
    (*env)->ReleaseStringUTFChars(env, system_name, c_system_name);
}

#else

bool android_is_rewind() {
    return false;
}

#endif
