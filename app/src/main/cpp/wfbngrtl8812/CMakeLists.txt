
cmake_minimum_required(VERSION 3.22.1)

project("wfbngrtl8812")

include_directories(include)

add_library(wfb-ng STATIC
        wfb-ng/src/fec.c
        wfb-ng/src/fec.h
        wfb-ng/src/ieee80211_radiotap.h
        wfb-ng/src/radiotap.c
        wfb-ng/src/rx.cpp
        wfb-ng/src/rx.hpp
        wfb-ng/src/wifibroadcast.hpp
        wfb-ng/src/wifibroadcast.cpp)
set_target_properties(wfb-ng PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/wfbngrtl8812/wfb-ng)
target_include_directories(wfb-ng PUBLIC wfb-ng)
target_compile_definitions(wfb-ng PRIVATE __WFB_RX_SHARED_LIBRARY__)

add_library(devourer STATIC
        devourer/hal/Hal8812PhyReg.h
        devourer/hal/Hal8812PwrSeq.c
        devourer/hal/Hal8812PwrSeq.h
        devourer/hal/basic_types.h
        devourer/hal/hal8812a_fw.c
        devourer/hal/hal8812a_fw.h
        devourer/hal/hal_com_reg.h
        devourer/hal/rtl8812a_hal.h
        devourer/hal/rtl8812a_recv.h
        devourer/hal/rtl8812a_spec.h

        devourer/src/EepromManager.cpp
        devourer/src/EepromManager.h
        devourer/src/Firmware.h
        devourer/src/FirmwareManager.cpp
        devourer/src/FirmwareManager.h
        devourer/src/FrameParser.cpp
        devourer/src/FrameParser.h
        devourer/src/HalModule.cpp
        devourer/src/HalModule.h
        devourer/src/ParsedRadioPacket.cpp
        devourer/src/RadioManagementModule.cpp
        devourer/src/RadioManagementModule.h
        devourer/src/Rtl8812aDevice.cpp
        devourer/src/Rtl8812aDevice.h
        devourer/src/RtlUsbAdapter.cpp
        devourer/src/RtlUsbAdapter.h
        devourer/src/SelectedChannel.h
        devourer/src/WiFiDriver.cpp
        devourer/src/WiFiDriver.h
        devourer/src/registry_priv.h)
set_target_properties(devourer PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/wfbngrtl8812/devourer)
target_include_directories(devourer PUBLIC devourer)
target_include_directories(devourer PUBLIC devourer/hal)
set_property(TARGET devourer PROPERTY CXX_STANDARD 20)



# WFB-NG RTL8812 library
add_library(wfbngrtl8812 SHARED
        RxFrame.h
        RxFrame.cpp
        WfbngLink.cpp)

target_link_libraries(wfbngrtl8812
        devourer
        wfb-ng
        android
        log
        ${CMAKE_SOURCE_DIR}/wfbngrtl8812/libs/${ANDROID_ABI}/libusb1.0.so
        ${CMAKE_SOURCE_DIR}/wfbngrtl8812/libs/${ANDROID_ABI}/libsodium.so
        ${CMAKE_SOURCE_DIR}/wfbngrtl8812/libs/${ANDROID_ABI}/libpcap.a)


target_include_directories(wfbngrtl8812 PUBLIC include)

set_property(TARGET wfbngrtl8812 PROPERTY CXX_STANDARD 20)
target_compile_options(wfbngrtl8812 PRIVATE -fno-omit-frame-pointer)