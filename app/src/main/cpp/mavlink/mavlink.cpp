#include <jni.h>
#include <string>

#include <sys/prctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <sys/prctl.h>
#include <sys/sem.h>
#include <thread>
#include <assert.h>
#include <android/log.h>

#include "mavlink/common/mavlink.h"
#include "mavlink.h"
#include "utils.h"

Mavlink::Mavlink(int port, std::function<void(mavlink_data)> cb): port_(port), callback_(cb) {}

int Mavlink::run() {
    __android_log_print(ANDROID_LOG_DEBUG, "mavlink.cpp", "Starting mavlink thread...");
    // Create socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "mavlink.cpp", "ERROR: Unable to create MavLink socket:  %s" , strerror(errno));
        return -1;
    }

    // Bind port
    struct sockaddr_in addr = {};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &(addr.sin_addr));
    addr.sin_port = htons(port_);

    if (bind(fd, (struct sockaddr*)(&addr), sizeof(addr)) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "mavlink.cpp", "ERROR: Unable to bind MavLink port: %s" , strerror(errno));
        return -1;
    }

    // Set Rx timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "mavlink.cpp", "ERROR: Unable to bind MavLink rx timeout:  %s" , strerror(errno));
        return -1;
    }

    char buffer[2048];
    char str[1024];
    while (!should_stop_) {
        memset(buffer, 0x00, sizeof(buffer));
        int ret = recv(fd, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            continue;
        } else if (ret == 0) {
            // peer has done an orderly shutdown
            return -1;
        }

        // Parse
        // Credit to openIPC:https://github.com/OpenIPC/silicon_research/blob/master/vdec/main.c#L1020
        mavlink_message_t message;
        mavlink_status_t status;
        bool data_update = false;
        mavlink_data latestMavlinkData;
        for (int i = 0; i < ret; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &message, &status) == 1) {
                switch (message.msgid) {
                    case MAVLINK_MSG_ID_HEARTBEAT:
                        // handle_heartbeat(&message);
                        break;

                    case MAVLINK_MSG_ID_SYS_STATUS:
                    {
                        mavlink_sys_status_t bat;
                        mavlink_msg_sys_status_decode(&message, &bat);
                        latestMavlinkData.telemetry_battery = bat.voltage_battery;
                        latestMavlinkData.telemetry_current = bat.current_battery;
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_BATTERY_STATUS:
                    {
                        mavlink_battery_status_t batt;
                        mavlink_msg_battery_status_decode(&message, &batt);
                        latestMavlinkData.telemetry_current_consumed = batt.current_consumed;
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
                    {
                        mavlink_rc_channels_raw_t rc_channels_raw;
                        mavlink_msg_rc_channels_raw_decode( &message, &rc_channels_raw);
                        latestMavlinkData.telemetry_rssi = rc_channels_raw.rssi;
                        latestMavlinkData.telemetry_throttle = (rc_channels_raw.chan4_raw - 1000) / 10;

                        if (latestMavlinkData.telemetry_throttle < 0) {
                            latestMavlinkData.telemetry_throttle = 0;
                        }
                        latestMavlinkData.telemetry_arm = rc_channels_raw.chan5_raw;
                        latestMavlinkData.telemetry_resolution = rc_channels_raw.chan8_raw;
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_GPS_RAW_INT:
                    {
                        mavlink_gps_raw_int_t gps;
                        mavlink_msg_gps_raw_int_decode(&message, &gps);
                        latestMavlinkData.telemetry_sats = gps.satellites_visible;
                        latestMavlinkData.telemetry_lat = gps.lat;
                        latestMavlinkData.telemetry_lon = gps.lon;
                        if (latestMavlinkData.telemetry_arm > 1700) {
                            if (latestMavlinkData.armed < 1) {
                                latestMavlinkData.armed = 1;
                                latestMavlinkData.telemetry_lat_base = latestMavlinkData.telemetry_lat;
                                latestMavlinkData.telemetry_lon_base = latestMavlinkData.telemetry_lon;
                            }

                            sprintf(latestMavlinkData.s1, "%.00f", latestMavlinkData.telemetry_lat);
                            if (latestMavlinkData.telemetry_lat < 10000000) {
                                insertString(latestMavlinkData.s1, "0.", 0);
                            }
                            if (latestMavlinkData.telemetry_lat > 9999999) {
                                if (numOfChars(latestMavlinkData.s1) == 8) {
                                    insertString(latestMavlinkData.s1, ".", 1);
                                } else {
                                    insertString(latestMavlinkData.s1, ".", 2);
                                }
                            }

                            sprintf(latestMavlinkData.s2, "%.00f", latestMavlinkData.telemetry_lon);
                            if (latestMavlinkData.telemetry_lon < 10000000) {
                                insertString(latestMavlinkData.s2, "0.", 0);
                            }
                            if (latestMavlinkData.telemetry_lon > 9999999) {
                                if (numOfChars(latestMavlinkData.s2) == 8) {
                                    insertString(latestMavlinkData.s2, ".", 1);
                                } else {
                                    insertString(latestMavlinkData.s2, ".", 2);
                                }
                            }

                            sprintf(latestMavlinkData.s3, "%.00f", latestMavlinkData.telemetry_lat_base);
                            if (latestMavlinkData.telemetry_lat_base < 10000000) {
                                insertString(latestMavlinkData.s3, "0.", 0);
                            }
                            if (latestMavlinkData.telemetry_lat_base > 9999999) {
                                if (numOfChars(latestMavlinkData.s3) == 8) {
                                    insertString(latestMavlinkData.s3, ".", 1);
                                } else {
                                    insertString(latestMavlinkData.s3, ".", 2);
                                }
                            }

                            sprintf(latestMavlinkData.s4, "%.00f", latestMavlinkData.telemetry_lon_base);
                            if (latestMavlinkData.telemetry_lon_base < 10000000) {
                                insertString(latestMavlinkData.s4, "0.", 0);
                            }

                            if (latestMavlinkData.telemetry_lon_base > 9999999) {
                                if (numOfChars(latestMavlinkData.s4) == 8) {
                                    insertString(latestMavlinkData.s4, ".", 1);
                                } else {
                                    insertString(latestMavlinkData.s4, ".", 2);
                                }
                            }

                            latestMavlinkData.s1_double = strtod(latestMavlinkData.s1, &latestMavlinkData.ptr);
                            latestMavlinkData.s2_double = strtod(latestMavlinkData.s2, &latestMavlinkData.ptr);
                            latestMavlinkData.s3_double = strtod(latestMavlinkData.s3, &latestMavlinkData.ptr);
                            latestMavlinkData.s4_double = strtod(latestMavlinkData.s4, &latestMavlinkData.ptr);
                        }
                        latestMavlinkData.telemetry_distance = distanceEarth(latestMavlinkData.s1_double, latestMavlinkData.s2_double, latestMavlinkData.s3_double, latestMavlinkData.s4_double);
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_VFR_HUD:
                    {
                        mavlink_vfr_hud_t vfr;
                        mavlink_msg_vfr_hud_decode(&message, &vfr);
                        latestMavlinkData.telemetry_gspeed = vfr.groundspeed * 3.6;
                        latestMavlinkData.telemetry_vspeed = vfr.climb;
                        latestMavlinkData.telemetry_altitude = vfr.alt;
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                    {
                        mavlink_global_position_int_t global_position_int;
                        mavlink_msg_global_position_int_decode( &message, &global_position_int);
                        latestMavlinkData.telemetry_hdg = global_position_int.hdg / 100;
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_ATTITUDE:
                    {
                        mavlink_attitude_t att;
                        mavlink_msg_attitude_decode(&message, &att);
                        latestMavlinkData.telemetry_pitch = att.pitch * (180.0 / 3.141592653589793238463);
                        latestMavlinkData.telemetry_roll = att.roll * (180.0 / 3.141592653589793238463);
                        latestMavlinkData.telemetry_yaw = att.yaw * (180.0 / 3.141592653589793238463);
                        data_update=true;
                    }
                        break;

                    case MAVLINK_MSG_ID_RADIO_STATUS:
                    {
                        if ((message.sysid != 3) || (message.compid != 68)) {
                            break;
                        }
                        latestMavlinkData.wfb_rssi = (int8_t)mavlink_msg_radio_status_get_rssi(&message);
                        latestMavlinkData.wfb_errors = mavlink_msg_radio_status_get_rxerrors(&message);
                        latestMavlinkData.wfb_fec_fixed = mavlink_msg_radio_status_get_fixed(&message);
                        latestMavlinkData.wfb_flags = mavlink_msg_radio_status_get_remnoise(&message);
                        data_update=true;
                    }
                        break;

                    default:
                        // printf("> MavLink message %d from %d/%d\n",
                        //   message.msgid, message.sysid, message.compid);
                        break;
                }
            }
        }

        if (data_update) {
            callback_(latestMavlinkData);
        }

        usleep(1);
    }

    __android_log_print(ANDROID_LOG_DEBUG, "mavlink.cpp", "Mavlink thread done.");
    return 0;
}

void Mavlink::stop() {
    should_stop_=true;
}
