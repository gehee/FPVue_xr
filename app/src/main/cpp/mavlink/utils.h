//
// Created by Gee He on 2024-05-01.
//

#ifndef FPVUE_XR_UTILS_H
#define FPVUE_XR_UTILS_H

#define earthRadiusKm 6371.0
#define BILLION 1000000000L

# define M_PI   3.14159265358979323846  /* pi */

double deg2rad(double degrees) {
    return degrees * M_PI / 180.0;
}

double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d) {
    double lat1r, lon1r, lat2r, lon2r, u, v;
    lat1r = deg2rad(lat1d);
    lon1r = deg2rad(lon1d);
    lat2r = deg2rad(lat2d);
    lon2r = deg2rad(lon2d);
    u = sin((lat2r - lat1r) / 2);
    v = sin((lon2r - lon1r) / 2);

    return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

size_t numOfChars(const char s[]) {
    size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }

    return n;
}

char* insertString(char s1[], const char s2[], size_t pos) {
    size_t n1 = numOfChars(s1);
    size_t n2 = numOfChars(s2);
    if (n1 < pos) {
        pos = n1;
    }

    for (size_t i = 0; i < n1 - pos; i++) {
        s1[n1 + n2 - i - 1] = s1[n1 - i - 1];
    }

    for (size_t i = 0; i < n2; i++) {
        s1[pos + i] = s2[i];
    }

    s1[n1 + n2] = '\0';

    return s1;
}

#endif //FPVUE_XR_UTILS_H
