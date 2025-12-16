#ifndef GPS_H
#define GPS_H

#ifdef __cplusplus
extern "C" {
#endif


struct gps_sample {
    int32_t lat_e7;     // degrees * 1e7
    int32_t lon_e7;     // degrees * 1e7
    int64_t ts_ms;      // k_uptime_get()
    bool    fix_valid;
};

int gps_init(void);
int get_gps_payload(struct gps_sample *out);

#ifdef __cplusplus
}
#endif

#endif // SENSORS_H
