//
// Created by gee on 2024-04-27.
//

#ifndef FPVUE_MAVLINK_H
#define FPVUE_MAVLINK_H

struct mavlink_data {
    // Mavlink
    float telemetry_altitude;
    float telemetry_pitch;
    float telemetry_roll;
    float telemetry_yaw;
    float telemetry_battery;
    float telemetry_current;
    float telemetry_current_consumed;
    double telemetry_lat;
    double telemetry_lon;
    double telemetry_lat_base;
    double telemetry_lon_base;
    double telemetry_hdg;
    double telemetry_distance;
    double s1_double;
    double s2_double;
    double s3_double;
    double s4_double;
    float telemetry_sats;
    float telemetry_gspeed;
    float telemetry_vspeed;
    float telemetry_rssi;
    float telemetry_throttle;
    float telemetry_resolution;
    float telemetry_arm;
    float armed;
    char c1[30];
    char c2[30];
    char s1[30];
    char s2[30];
    char s3[30];
    char s4[30];
    char* ptr;
    int8_t wfb_rssi;
    uint16_t wfb_errors;
    uint16_t wfb_fec_fixed;
    int8_t wfb_flags;
};

class Mavlink{
    public:
        Mavlink(int port, std::function<void(mavlink_data)> callback);
        int run();
        void stop();

    private:
        int port_;
        bool should_stop_ = false;
        std::function<void(mavlink_data)> callback_;
};

#endif //FPVUE_MAVLINK_H
