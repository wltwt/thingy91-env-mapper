#pragma once

#include <stdint.h>

#define COMPANY_ID_NORDIC 0x0059
#define PROTO_MAGIC       0xA5
#define PROTO_VER         1

struct node_b_sample {
    int64_t ts_ms;
    uint16_t seq;
    int16_t  t_c_e2;
    uint16_t rh_e2;
    int32_t  lat_e7;
    int32_t  lon_e7;
    uint8_t  src;
    int8_t   rssi;
};

/* Manufacturer data format – Thingy Node A */
typedef struct __packed {
    uint16_t company_code;
    uint8_t  magic;
    uint8_t  ver;
    uint16_t seq;
    int16_t  t_c_e2;
    uint16_t rh_e2;
    int32_t  lat_e7;
    int32_t  lon_e7;
    uint8_t  src;
} adv_mfg_data_type;
