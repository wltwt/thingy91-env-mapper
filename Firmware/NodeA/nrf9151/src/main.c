#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>


#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <string.h>


#include "sensors.h"
#include "gps.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#define PKT_MAGIC 0xA5
#define PKT_VER   1

struct __packed env_pkt_wire {
    uint8_t  magic;        // 0xA5
    uint8_t  ver;          // 1
    uint16_t seq_le;       // little-endian
    int16_t  t_c_e2_le;    // temp * 100
    uint16_t rh_e2_le;     // RH * 100
    int32_t  lat_e7_le;    // lat * 1e7
    int32_t  lon_e7_le;    // lon * 1e7
    uint8_t  src;          // team id
    uint8_t  crc8;         // over bytes [magic..src]
};

BUILD_ASSERT(sizeof(struct env_pkt_wire) == 1+1+2+2+2+4+4+1+1, "pkt size changed");



static uint8_t crc8_atm(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

//LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
static uint16_t g_seq;

static void build_pkt(struct env_pkt_wire *p,
                      int16_t t_c_e2, uint16_t rh_e2,
                      int32_t lat_e7, int32_t lon_e7,
                      uint8_t src)
{
    p->magic = PKT_MAGIC;
    p->ver   = PKT_VER;
    p->seq_le    = sys_cpu_to_le16(g_seq++);
    p->t_c_e2_le = sys_cpu_to_le16((uint16_t)t_c_e2);   // cast ok
    p->rh_e2_le  = sys_cpu_to_le16(rh_e2);
    p->lat_e7_le = sys_cpu_to_le32(lat_e7);
    p->lon_e7_le = sys_cpu_to_le32(lon_e7);
    p->src = src;

    // CRC over alt uten crc8-feltet
    p->crc8 = crc8_atm((const uint8_t*)p, offsetof(struct env_pkt_wire, crc8));
}


static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));  // evt uart1

static void uart_send_pkt_poll(const struct device *uart, const struct env_pkt_wire *p)
{
    const uint8_t *b = (const uint8_t*)p;
    for (size_t i = 0; i < sizeof(*p); i++) {
        uart_poll_out(uart, b[i]);
    }
}

static void send_str(const char *s)
{
    while (*s) {
        uart_poll_out(uart, *s++);
    }
}


int main(void)
{
    //LOG_INF("App boot");

    /*
    int err = sensors_init();
    LOG_INF("sensors_init returned %d", err);

    int gps = run_gps();
    */

    //while (1) {
    //    int val = sensors_sample_once();
    //    LOG_INF("main got %d", val);
    //    k_sleep(K_SECONDS(2));
    //}

    if (!device_is_ready(uart)) return 0;
    
    struct env_pkt_wire pkt;
    
    int16_t  t_c_e2 = 2231;        // 22.31°C
    uint16_t rh_e2  = 4890;        // 48.90%
    int32_t  lat_e7 = 599139000;   // 59.9139000
    int32_t  lon_e7 = 107522000;   // 10.7522000
    uint8_t  src    = 3;
    
    build_pkt(&pkt, t_c_e2, rh_e2, lat_e7, lon_e7, src);
    
    
    while (1) {
        uart_send_pkt_poll(uart, &pkt);
        k_sleep(K_SECONDS(1));
    }
}
