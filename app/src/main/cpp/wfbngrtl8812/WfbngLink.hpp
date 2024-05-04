#ifndef FPV_VR_WFBNG_LINK_H
#define FPV_VR_WFBNG_LINK_H

#include <jni.h>
#include "wfb-ng/src/rx.hpp"
#include "devourer/src/WiFiDriver.h"

class WfbngLink{
public:
    WfbngLink(JNIEnv * env, int fd, const char *key);
    int run(JNIEnv *env, int wifiChannel);
    void stop(JNIEnv *env);
    Aggregator* aggregator;

private:
    const char *keyPath;
    int fd;
    std::unique_ptr<Rtl8812aDevice> rtlDevice;
    bool should_stop;
};

#endif //FPV_VR_WFBNG_LINK_H
