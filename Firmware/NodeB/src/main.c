#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>

#include <zephyr/kernel.h>


#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

LOG_MODULE_REGISTER(node_b, LOG_LEVEL_INF);

/* ================= CONFIG ================= */

#define COMPANY_ID_NORDIC 0x0059
#define PROTO_MAGIC       0xA5
#define PROTO_VER         1

static int8_t last_rssi;

/* ================= DATA TYPES ================= */

static void wait_for_usb_console(uint32_t timeout_ms)
{
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;
    uint32_t t0 = k_uptime_get();

    if (!device_is_ready(dev)) {
        return;
    }

    while (!dtr && (k_uptime_get() - t0 < timeout_ms)) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(50));
    }
}



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


/* ================== Debug printer =================*/
static void print_raw_thingy_packet(const adv_mfg_data_type *p)
{
    printk("RAW FIELDS:\n");
    printk("  company_code : 0x%04X\n", sys_le16_to_cpu(p->company_code));
    printk("  magic        : 0x%02X\n", p->magic);
    printk("  ver          : %u\n", p->ver);
    printk("  seq          : %u\n", sys_le16_to_cpu(p->seq));
    printk("  t_c_e2       : %d\n", sys_le16_to_cpu(p->t_c_e2));
    printk("  rh_e2        : %u\n", sys_le16_to_cpu(p->rh_e2));
    printk("  lat_e7       : %d\n", sys_le32_to_cpu(p->lat_e7));
    printk("  lon_e7       : %d\n", sys_le32_to_cpu(p->lon_e7));
    printk("  src          : %u\n", p->src);
}

/* ================= MESSAGE QUEUE ================= */

K_MSGQ_DEFINE(sample_q, sizeof(struct node_b_sample), 8, 8);

/* ================= SAMPLE HANDLING ================= */

static void sample_to_json(const struct node_b_sample *s,
                           char *buf, size_t len)
{
    snprintf(buf, len,
        "{\"ts\":%u,"
        "\"seq\":%u,"
        "\"t_c_e2\":%d,"
        "\"rh_e2\":%u,"
        "\"lat_e7\":%d,"
        "\"lon_e7\":%d,"
        "\"src\":%u,"
        "\"rssi\":%d}",
        (uint32_t)s->ts_ms,   // 32-bit er OK
        s->seq,
        s->t_c_e2,
        s->rh_e2,
        s->lat_e7,
        s->lon_e7,
        s->src,
        s->rssi);
}




static void handle_sample(struct node_b_sample *s)
{
    char json[192];

    sample_to_json(s, json, sizeof(json));

    printk("JSON %s\n", json);
}

void sample_worker(void)
{
    struct node_b_sample s;

    while (1) {
        k_msgq_get(&sample_q, &s, K_FOREVER);
        handle_sample(&s);
    }
}

K_THREAD_DEFINE(sample_thread, 2048, sample_worker,
                NULL, NULL, NULL, 5, 0, 0);

/* ================= PARSER ================= */

static void parse_adv_mfg_v1(const uint8_t *data, uint8_t len, int8_t rssi)
{
    if (len != sizeof(adv_mfg_data_type)) {
        return;
    }

    const adv_mfg_data_type *p =
        (const adv_mfg_data_type *)data;

    if (sys_le16_to_cpu(p->company_code) != COMPANY_ID_NORDIC ||
        p->magic != PROTO_MAGIC ||
        p->ver   != PROTO_VER) {
        return;
    }

    struct node_b_sample s = {
        .ts_ms  = k_uptime_get(),
        .seq    = sys_le16_to_cpu(p->seq),

        // Store raw data
        .t_c_e2 = sys_le16_to_cpu(p->t_c_e2),
        .rh_e2  = sys_le16_to_cpu(p->rh_e2),

        .lat_e7 = sys_le32_to_cpu(p->lat_e7),
        .lon_e7 = sys_le32_to_cpu(p->lon_e7),

        .src    = p->src,
        .rssi   = rssi
    };

    k_msgq_put(&sample_q, &s, K_NO_WAIT);
}


/* ================= AD PARSER ================= */

static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
    ARG_UNUSED(user_data);

    if (data->type != BT_DATA_MANUFACTURER_DATA) {
        return true;
    }

    /* Filter 1: Company ID */
    if (sys_get_le16(data->data) != COMPANY_ID_NORDIC) {
        return true;
    }

    /* Filter 2: Lengde */
    if (data->data_len != sizeof(adv_mfg_data_type)) {
        return true;
    }

    const adv_mfg_data_type *p =
        (const adv_mfg_data_type *)data->data;

    /* Filter 3: Magic + version */
    if (p->magic != PROTO_MAGIC || p->ver != PROTO_VER) {
        return true;
    }

    printk("\n=== THINGY NODE A PACKET ===\n");

    //DEBUG RAW DATA
    /*
    // 
    printk("RAW BYTES: ");
    for (int i = 0; i < data->data_len; i++) {
        printk("%02X ", data->data[i]);
    }
    printk("\n");

    // 
    print_raw_thingy_packet(p);
    */

    // Send data to json format
    parse_adv_mfg_v1(data->data, data->data_len, last_rssi);

    return false; /* ferdig med denne AD */
}

/* ================= SCAN CALLBACK ================= */

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi,
                    uint8_t adv_type, struct net_buf_simple *buf)
{
    ARG_UNUSED(addr);
    ARG_UNUSED(adv_type);

    last_rssi = rssi;
    bt_data_parse(buf, ad_parse_cb, NULL);
}

/* ================= MAIN ================= */

int main(void)
{
    int err;

    // Start USB (CDC ACM)
    err = usb_enable(NULL);

    if (err) {
        return 0;
    }
    // Wait for terminal to connect
    wait_for_usb_console(3000);


    printk("Node B: Thingy scanner starting");

    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (%d)", err);
        return 0;
    }

    struct bt_le_scan_param scan_param = {
        .type     = BT_LE_SCAN_TYPE_PASSIVE,
        .options  = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window   = BT_GAP_SCAN_FAST_WINDOW,
    };

    err = bt_le_scan_start(&scan_param, scan_cb);
    if (err) {
        printk("Scan start failed (%d)", err);
        return 0;
    }

    printk("Scanning for Thingy Node A");

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
