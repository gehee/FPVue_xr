#include <stereokit.h>
#include <stereokit_ui.h>
#include <random>
#include <jni.h>

#include <android/log.h>
#include <android/asset_manager_jni.h>
#include "videonative/VideoPlayer.h"
#include <opencv2/opencv.hpp>
#include <time.h>
#include "wfbngrtl8812/WfbngLink.hpp"
#include "mavlink/mavlink.h"

using namespace sk;
using namespace std::chrono;

int32_t rtl8812UsbPath(JNIEnv* env,struct android_app *app);
std::string copyGsKey(JNIEnv * env, android_app *app);

#define NANOS_IN_SECOND 1000000000
static long currentTimeInNanos() {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return (res.tv_sec * NANOS_IN_SECOND) + res.tv_nsec;
}

// Video conversion
mesh_t     plane_mesh;
material_t plane_mat;
tex_t vid0;
cv::Mat buffer0;

// Screen size
float screen_width = 2.0;
float screen_aspect_ratio = 9.0/16.0;

// Video stats
int32_t video_width = 0;
int32_t video_height = 0;
int framerate = 0;
int screen_refresh_rate = 0;
bool no_adapter = 0;

int decoded_frame_period = 0;
time_point decoded_period_start = steady_clock::now();
int displayed_frame_period = 0;
time_point displayed_period_start = steady_clock::now();
duration fps_log_interval = std::chrono::seconds(1);

struct mavlink_data mavlink_data;

void onNewFrame(const uint8_t* data,const std::size_t data_length, int32_t width, int32_t height) {
    video_width = width; video_height = height;
    cv::Mat yuv(height + height / 2, width, CV_8UC1, const_cast<uint8_t*>(data)); //
    cv::Mat rgbFrame;
    cv::cvtColor(yuv, rgbFrame, cv::COLOR_YUV2BGR_NV21);
    cv::cvtColor(rgbFrame, buffer0, cv::COLOR_RGB2RGBA);
    decoded_frame_period++;
    auto now=steady_clock::now();
    if ((now-decoded_period_start)>fps_log_interval) {
        framerate = decoded_frame_period;
        decoded_period_start = now;
        decoded_frame_period = 0;
    }
}

bool app_init(struct android_app *state)
{
    JavaVM* lJavaVM = state->activity->vm;
    JNIEnv* env = nullptr;
    bool lThreadAttached = false;
    // Get JNIEnv from lJavaVM using GetEnv to test whether
    // thread is attached or not to the VM. If not, attach it
    // (and note that it will need to be detached at the end
    //  of the function).
    switch (lJavaVM->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK: break;
        case JNI_EVERSION: return -1;
        case JNI_EDETACHED: {
            jint lResult = lJavaVM->AttachCurrentThread(&env, nullptr);
            if(lResult == JNI_ERR) {
                return -1;
            }
            lThreadAttached = true;
        } break;
    }

    log_set_filter(log_error); // log_error shows problem
    sk_settings_t settings = {};

    settings.app_name = "FPVue";
    settings.android_activity = state->activity->clazz;
    settings.android_java_vm = state->activity->vm;
    settings.display_preference = display_mode_mixedreality;

    if (!sk_init(settings))
    {
        return false;
    }

    // WFB ng
    auto key_path = copyGsKey(env, state);
    if (key_path == "" ){
        return false;
    }
    auto fd = rtl8812UsbPath(env, state);
    if (fd > 0) {
        std::thread wfb_thread([&fd, &env, key_path]() {
            WfbngLink wfb(env, fd,  key_path.c_str());
            wfb.run(env, 149);
        });
        wfb_thread.detach();
    } else {
        no_adapter = true;
    }

    // Mavlink
    auto updateMavlinkData([](struct mavlink_data data) {
        mavlink_data = data;
    });
    std::thread mavlink_thread( [&updateMavlinkData]() {
        Mavlink mavlink(14550, updateMavlinkData);
        mavlink.run();
    });
    mavlink_thread.detach();

    // Video decoding
    plane_mesh = mesh_gen_plane({screen_width,screen_width*screen_aspect_ratio},{0,0,1},{0,1,0});
    plane_mat = material_copy_id(default_id_material_unlit);
    vid0 = tex_create(tex_type_image_nomips,tex_format_rgba32);
    tex_set_address(vid0, tex_address_clamp);
    material_set_texture(plane_mat,"diffuse",vid0);

    auto* p= new VideoPlayer(std::bind(onNewFrame,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4));
    p->start();
    return true;
}

void app_handle_cmd(android_app *evt_app, int32_t cmd)
{
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONFIG_CHANGED:
        sk_set_window(evt_app->window);
        break;
    case APP_CMD_TERM_WINDOW:
        sk_set_window(nullptr);
        break;
    }
}

pose_t plane_pose = {{0, 0.05,-1.5f}, {0,0,0,1}};

void app_step()
{
    sk_step([]() {
        displayed_frame_period++;
        auto now = steady_clock::now();
        if ((now-displayed_period_start)>fps_log_interval) {
            screen_refresh_rate = displayed_frame_period;
            displayed_period_start = now;
            displayed_frame_period = 0;
        }
        if(no_adapter) {
            std::string txt = "No wifi adapter.";
            text_add_at(txt.c_str(),
                        matrix_trs({-0.1, -0.1, -1.4f}, quat_identity, vec3{-1.0f, 1.0f, 1.0f}),
                        0, text_align_bottom_left);
        } else {
            if (!buffer0.empty()) {
                tex_set_colors(vid0, buffer0.cols, buffer0.rows, (void *) buffer0.datastart);
                ui_handle_begin("Plane", plane_pose, mesh_get_bounds(plane_mesh), false);
                render_add_mesh(plane_mesh, plane_mat, matrix_identity);
                ui_handle_end();
            } else {
                std::string txt = "No data.";
                text_add_at(txt.c_str(),
                            matrix_trs({-0.1, -0.1, -1.4f}, quat_identity, vec3{-1.0f, 1.0f, 1.0f}),
                            0, text_align_bottom_left);
            }
        }

        std::string txt = "" + std::to_string(video_width) + "x" + std::to_string(video_height) + "\t" + std::to_string(framerate) + "fps\t" +  std::to_string(screen_refresh_rate) + " Hz";
        text_add_at(txt.c_str(), matrix_trs({-0.1,-0.52,-1.4f}, quat_identity, vec3{-1.0f, 1.0f, 1.0f}), 0, text_align_bottom_left);
        std::string txt1 = "" + std::to_string(mavlink_data.telemetry_battery/1000.0) + "V";
        text_add_at(txt1.c_str(), matrix_trs({-0.4,-0.52,-1.4f}, quat_identity, vec3{-1.0f, 1.0f, 1.0f}), 0, text_align_bottom_left);
    });
}

void app_exit()
{
    sk_shutdown();
}

std::string copyGsKey(JNIEnv * env, android_app *app) {
    jclass natact  = env->FindClass("android/app/NativeActivity");
    jmethodID getAssetManagerMethod = env->GetMethodID(natact, "getAssets", "()Landroid/content/res/AssetManager;");
    auto thiz = app->activity->clazz;
    jobject assetManager = env->CallObjectMethod(thiz , getAssetManagerMethod);

    // Get the asset manager
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (mgr == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "copyGsKey", "Failed to obtain AssetManager");
        return "";
    }

    // Open the asset file
    AAsset* asset = AAssetManager_open(mgr, "gs.key", AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "copyGsKey", "Failed to open asset file");
        return "";
    }

    jmethodID getApplicationContextMethod = env->GetMethodID(natact, "getApplicationContext", "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(thiz , getApplicationContextMethod);

    jmethodID getFilesDirMethod = env->GetMethodID(env->GetObjectClass(context), "getFilesDir", "()Ljava/io/File;");
    jobject filesDirObject = env->CallObjectMethod(context, getFilesDirMethod);
    jmethodID getAbsolutePathMethod = env->GetMethodID(env->GetObjectClass(filesDirObject), "getAbsolutePath", "()Ljava/lang/String;");
    jstring filesDirStr = (jstring)env->CallObjectMethod(filesDirObject, getAbsolutePathMethod);
    const char* filesDir = env->GetStringUTFChars(filesDirStr, nullptr);
    // Create destination file path
    //char destinationFilePath[256];
    std::stringstream destinationFilePath;
    destinationFilePath << filesDir << "/" << "gs.key";
    //snprintf(destinationFilePath, sizeof(destinationFilePath), "%s/gs.key", filesDir);
    env->ReleaseStringUTFChars(filesDirStr, filesDir);

    // Open the destination file
    FILE* destinationFile = fopen(destinationFilePath.str().c_str(), "wb");
    if (destinationFile == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "copyGsKey", "Failed to open destination file");
        AAsset_close(asset);
        return "";
    }

    // Copy asset contents to the destination file
    const void* buffer;
    off_t bufferSize;
    buffer = AAsset_getBuffer(asset);
    bufferSize = AAsset_getLength(asset);
    fwrite(buffer, bufferSize, 1, destinationFile);

    // Close files
    fclose(destinationFile);
    AAsset_close(asset);

    __android_log_print(ANDROID_LOG_ERROR, "copyGsKey", "gs.key copied to %s", destinationFilePath.str().c_str());

    return destinationFilePath.str();
}

int32_t rtl8812UsbPath(JNIEnv* env,struct android_app *app) {

    jclass natact  = env->FindClass("android/app/NativeActivity");
    jmethodID getSystemServiceMethod = env->GetMethodID(natact , "getSystemService",
                                                        "(Ljava/lang/String;)Ljava/lang/Object;");
    jstring usbServiceName = env->NewStringUTF("usb");
    jobject usbManager = env->CallObjectMethod(app->activity->clazz , getSystemServiceMethod, usbServiceName);
    env->DeleteLocalRef(usbServiceName);
    if (usbManager == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "android_usb_devices", "UsbManager not available");
        return -1;
    }

    // Get device list
    jclass usbManagerClass = env->GetObjectClass(usbManager);
    jmethodID getDeviceListMethod = env->GetMethodID(usbManagerClass, "getDeviceList", "()Ljava/util/HashMap;");
    jobject deviceListObj = env->CallObjectMethod(usbManager, getDeviceListMethod);
    if (deviceListObj == nullptr) {
        __android_log_write(ANDROID_LOG_ERROR, "android_usb_devices", "Device list is null");
        return -1;
    }

    // Convert Java HashMap to C++ std::map
    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID keySetMethod = env->GetMethodID(mapClass, "keySet", "()Ljava/util/Set;");
    jobject keySetObj = env->CallObjectMethod(deviceListObj, keySetMethod);
    jclass setClass = env->FindClass("java/util/Set");
    jmethodID toArrayMethod = env->GetMethodID(setClass, "toArray", "()[Ljava/lang/Object;");
    jobjectArray keyArray = (jobjectArray) env->CallObjectMethod(keySetObj, toArrayMethod);
    jint keyCount = env->GetArrayLength(keyArray);

    std::map<std::string, jobject> deviceMap;

    for (int i = 0; i < keyCount; ++i) {
        jobject key = env->GetObjectArrayElement(keyArray, i);
        jstring keyString = (jstring) key;
        const char *keyChars = env->GetStringUTFChars(keyString, nullptr);
        jobject value = env->CallObjectMethod(deviceListObj, env->GetMethodID(mapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;"), key);
        deviceMap[std::string(keyChars)] = value;
        env->ReleaseStringUTFChars(keyString, keyChars);
        //env->DeleteLocalRef(keyString);
        //env->DeleteLocalRef(key);
    }

    const char* result ;
    // Iterate through the device map
    for (auto &device : deviceMap) {
        jobject usbDevice = device.second;
        jclass usbDeviceClass = env->GetObjectClass(usbDevice);
        jmethodID getDeviceNameMethod = env->GetMethodID(usbDeviceClass, "getDeviceName", "()Ljava/lang/String;");
        jstring deviceName = (jstring) env->CallObjectMethod(usbDevice, getDeviceNameMethod);
        const char *deviceNameChars = env->GetStringUTFChars(deviceName, nullptr);

        __android_log_print(ANDROID_LOG_ERROR, "android_usb_devices", "Opening %s", deviceNameChars);
        jmethodID openDeviceMethod = env->GetMethodID(usbManagerClass, "openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;" );
        jobject deviceConnection = env->CallObjectMethod(usbManager, openDeviceMethod, usbDevice);

        jclass deviceConnectionClass = env->GetObjectClass(deviceConnection);
        jmethodID getFileDescriptor = env->GetMethodID( deviceConnectionClass, "getFileDescriptor", "()I" );
        jint deviceConnectionFD = env->CallIntMethod( deviceConnection, getFileDescriptor );

        __android_log_print(ANDROID_LOG_ERROR, "android_usb_devices", "Gof fd=%d", deviceConnectionFD);

        return deviceConnectionFD;
    }
    return -1;
}