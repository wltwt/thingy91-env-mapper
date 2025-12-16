#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/sys/byteorder.h>


LOG_MODULE_REGISTER(node_b, LOG_LEVEL_INF);

// Config 
#define COMPANY_ID 0xFFFF
#define NODE_ID_A 1 


// Manufacturer data format - NodeA
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



// Manufacturer parser 
static bool ad_parse_cb(struct bt_data *data, void *user_data)
{
        /* data */
        ARG_UNUSED(user_data);

        if (data->type != BT_DATA_MANUFACTURER_DATA){
                return true; // continue parsing
        }

        if (data->data_len < 6) {
                LOG_WRN("Manufacturer data too short (%u)", data->data_len);
                return false; 
        }

        const uint8_t *d = data->data; 

        /* NOTE: 
        * Current parsing matches ESP32 Node A 
        * When Node A is available, replace this section 
        * with parsing og adv_mfg_data_type (see README)
        */

        uint16_t company_id = sys_get_le16(&d[0]); 
        uint8_t node_id = d[2];
        uint8_t seq = d[3]; 
        int16_t temp_raw = sys_get_le16(&d[4]); 

        if (company_id !=  COMPANY_ID || node_id != NODE_ID_A){
                return false; 
        }

        LOG_INF("Node A | seq=%u | temp_raw=%d (%.2f C)",
                seq,
                temp_raw,
                temp_raw / 100.0f);

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
