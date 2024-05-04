cmake_minimum_required(VERSION 3.18.1)

project("fpvue_xr")

add_library(openxr_loader SHARED IMPORTED)
set_target_properties(openxr_loader PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/bin/${ANDROID_ABI}/Release/libopenxr_loader.so)
set_target_properties(openxr_loader PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/include/openxr)

add_library(reactphysics3d STATIC IMPORTED)
set_target_properties(reactphysics3d PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/bin/${ANDROID_ABI}/Release/libreactphysics3d.a)
set_target_properties(reactphysics3d PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/include/reactphysics3d)

file(GLOB_RECURSE STEREOKIT_SRCS ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/*.cpp)
add_library(stereokitc SHARED ${STEREOKIT_SRCS})
set_target_properties(stereokitc PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC)
target_include_directories(stereokitc PUBLIC ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/include ${CMAKE_SOURCE_DIR}/StereoKit/StereoKitC/lib/include_no_win)
target_link_libraries(stereokitc
        android
        EGL
        GLESv1_CM
        log
        openxr_loader
        reactphysics3d)

add_library(videonative STATIC ${CMAKE_SOURCE_DIR}/videonative/parser/H26XParser.cpp
        ${CMAKE_SOURCE_DIR}/videonative/parser/ParseRTP.cpp
        ${CMAKE_SOURCE_DIR}/videonative/UdpReceiver.cpp
        ${CMAKE_SOURCE_DIR}/videonative/VideoDecoder.cpp
        ${CMAKE_SOURCE_DIR}/videonative/VideoPlayer.cpp)
set_target_properties(videonative PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/videonative)
target_include_directories(videonative PUBLIC ${CMAKE_SOURCE_DIR}/videonative)

## wfbngrtl8812
add_subdirectory(wfbngrtl8812)

## mavlink
add_library(mavlink STATIC ${CMAKE_SOURCE_DIR}/mavlink/mavlink.cpp)

# OpenCV
include(FetchContent)
FetchContent_Declare(
        opencv
        URL     https://github.com/opencv/opencv/releases/download/4.9.0/opencv-4.9.0-android-sdk.zip
)
FetchContent_GetProperties(opencv)
if(NOT opencv_POPULATED)
    message(STATUS "Fetching OpenCV SDK for Android")
    FetchContent_Populate(opencv)
    message(STATUS "Fetching OpenCV SDK for Android - done")
endif()
find_package(OpenCV REQUIRED PATHS ${opencv_SOURCE_DIR}/sdk/native/jni NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
add_library(opencv INTERFACE IMPORTED GLOBAL)


add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
set_target_properties(native_app_glue PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${ANDROID_NDK}/sources/android/native_app_glue)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(native-activity SHARED main.cpp app.cpp)

target_precompile_headers(native-activity PUBLIC "pch.h")

target_link_libraries(native-activity
        android
        mediandk
        log
        native_app_glue
        EGL
        GLESv1_CM
        log
        mavlink
        wfbngrtl8812
        stereokitc
        videonative
        ${OpenCV_LIBS})