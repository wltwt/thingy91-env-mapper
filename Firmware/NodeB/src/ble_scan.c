#include "ble_scan.h"
#include "protocol.h"
#include "sample.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>

static int8_t last_rssi;

static void parse_adv_mfg_v1(const uint8_t *data, uint8_t len, int8_t rssi)
{
    if (len < sizeof(adv_mfg_data_type)) {
        return;
    }

    const adv_mfg_data_type *p = (const adv_mfg_data_type *)data;

    if (sys_le16_to_cpu(p->company_code) != COMPANY_ID_NORDIC ||
        p->magic != PROTO_MAGIC ||
        p->ver   != PROTO_VER) {
        return;
    }

    struct node_b_sample s = {
        .ts_ms  = k_uptime_get(),
        .seq    = sys_le16_to_cpu(p->seq),
        .t_c_e2 = sys_le16_to_cpu(p->t_c_e2),
        .rh_e2  = sys_le16_to_cpu(p->rh_e2),
        .lat_e7 = sys_le32_to_cpu(p->lat_e7),
        .lon_e7 = sys_le32_to_cpu(p->lon_e7),
        .src    = p->src,
        .rssi   = rssi
    };

    sample_push(&s);
}

static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
    ARG_UNUSED(user_data);

    // printk("AD type %u len %u\n", data->type, data->data_len); DEBUG

    if (data->type != BT_DATA_MANUFACTURER_DATA) {
        return true;
    }

    parse_adv_mfg_v1(data->data, data->data_len, last_rssi);
    return true;
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi,
                    uint8_t adv_type, struct net_buf_simple *buf)
{
    ARG_UNUSED(addr);
    ARG_UNUSED(adv_type);

    // printk("scan_cb RSSI %d len %u\n", rssi, buf->len); Debug

    last_rssi = rssi;
    bt_data_parse(buf, ad_parse_cb, NULL);
}

int ble_scan_start(void)
{
    struct bt_le_scan_param scan_param = {
        .type     = BT_LE_SCAN_TYPE_PASSIVE,
        .options  = 0,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window   = BT_GAP_SCAN_FAST_WINDOW,
    };

    return bt_le_scan_start(&scan_param, scan_cb);
}
