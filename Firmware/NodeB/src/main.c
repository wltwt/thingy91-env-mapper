#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>


LOG_MODULE_REGISTER(node_b, LOG_LEVEL_INF);

// Config 
#define COMPANY_ID 0xFFFF
#define NODE_ID_A 1 

#define PROTO_MAGIC 0x42
#define PROTO_VER 1


// Manufacrurer data format struct - ESP32
typedef struct __packed legacy_payload {
    uint16_t company_id;
    uint8_t  node_id;
    uint8_t  seq;
    int16_t  temperature;
} legacy_payload_t;


// Manufacturer data format struct - NodeA
typedef struct __packed adv_mfg_data {

    uint16_t company_code; /* Company Identifier Code */
    uint8_t  magic;        /* Protocol identifier */
    uint8_t  ver;          /* Protocol version */
    uint16_t seq;          /* Sequence number */
    int16_t  t_c_e2;       /* Temperature (°C ×100) */
    uint16_t rh_e2;        /* Humidity (% ×100) */
    int32_t  lat_e7;       /* Latitude ×1e7 */
    int32_t  lon_e7;       /* Longitude ×1e7 */
    uint8_t  src;          /* Source / team number */

} adv_mfg_data_type;



// Parsec function for ESP32
static void parse_legacy_esp32(const uint8_t *data, uint8_t len){
        if (len < sizeof(legacy_payload_t)){
                LOG_WRN("Legacy payload too short (%u)", len);
                return;
        }

        legacy_payload_t p;
        memcpy(&p, data, sizeof(p));

        if (p.company_id != COMPANY_ID || p.node_id != NODE_ID_A){
                return;
        }

        LOG_INF("Legacy ESP32 | seq=%u | temp=%.2f C",
                p.seq,
                p.temperature / 100.0f);
}


// Parsec function for Node A

static void parse_adv_mfg_v1(const uint8_t *data, uint8_t len){
        if (len < sizeof(adv_mfg_data_type)){
                LOG_WRN("adv_mfg_data_type too short (%u)", len);
                return; 
        }

        adv_mfg_data_type p;
        memcpy(&p, data, sizeof(p));

        if (p.company_code != COMPANY_ID){
                return; 
        }

        if (p.magic != PROTO_MAGIC || p.ver != PROTO_VER){
                LOG_WRN("Unknown protocol magic=0x%02X ver=%u", p.magic, p.ver);
                return;
        }

        LOG_INF("ADV v1 | src=%u | seq=%u | T=%.2fC | RH=%0.2f%%",
                p.src,
                p.seq,
                p.t_c_e2 / 100.0f,
                p.rh_e2 / 100.0f);
}



// Manufacturer parser 
static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
        /* data */
        ARG_UNUSED(user_data);

        if (data->type != BT_DATA_MANUFACTURER_DATA){
                return true; // continue parsing
        }


        // Dispatch based on payload length
        if (data->data_len == sizeof(legacy_payload_t)) {
                parse_legacy_esp32(data->data, data->data_len);
                
        }
        else if (data->data_len >= sizeof(adv_mfg_data_type)) {
                parse_adv_mfg_v1(data->data, data->data_len);
        }
        else{
                LOG_WRN("Unknown manufacturer payload length (%u)", data->data_len);
        }

        return false; // Stop parsing this packet
}



// Callback: Advertising packet is recieved
static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple *buf)
{
        ARG_UNUSED(addr);
        ARG_UNUSED(rssi);
        ARG_UNUSED(adv_type); 

        bt_data_parse(buf, ad_parse_cb, NULL);
}
                        

int main(void)
{
        int err; 

        LOG_INF("Node B: Starting BLE scanner"); 

        // Enable Bluetooth
        err = bt_enable(NULL); 
        if (err) {
                LOG_ERR("Bluetooth init failed (err %d)", err);
                return 0;
        }

        LOG_INF("Bluetooth initialized");


        // Replace to: Using adv_mfg_data_type protocol 
        LOG_INF("NOTE: Using legacy ESP32 manufacturer format");


        // Scan parameters
        struct bt_le_scan_param scan_param =
        {
                /* data */
                .type = BT_LE_SCAN_TYPE_PASSIVE,
                .options = BT_LE_SCAN_OPT_NONE,
                .interval = BT_GAP_SCAN_FAST_INTERVAL,
                .window = BT_GAP_SCAN_FAST_WINDOW,
        };


        // Start scanning
        err = bt_le_scan_start(&scan_param, scan_cb); 
        if (err){ 
                LOG_ERR("Scanning failed to start (err %d)", err);
                return 0;
        }

        LOG_INF("Scanning for ESP32 Node A");

        // No main loop everything happens in callback

        while(1){
                k_sleep(K_SECONDS(1)); 
        }
}
