#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


LOG_MODULE_REGISTER(rtt_test, LOG_LEVEL_INF);

// Callback: Advertising packet is recieved
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad)
{
        char addr_str[BT_ADDR_LE_STR_LEN];

        bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

        LOG_INF("Device found: %s | RSSI %d | AD len %u", addr_str, rssi, ad->len);
};
                        

int main(void)
{
        int err; 

        printk("Node B: RTT + BLE scan starting\n"); 
        LOG_INF("RTT logging works"); 

        // Enable Bluetooth
        err = bt_enable(NULL); 
        if (err) {
                LOG_ERR("Bluetooth init failed (err %d)", err);
                return 0;
        }

        // Scan parameters
        struct bt_le_scan_param scan_param =
        {
                /* data */
                .type = BT_LE_SCAN_TYPE_PASSIVE,
                .options = BT_LE_SCAN_OPT_NONE,
                .interval = BT_GAP_SCAN_FAST_WINDOW,
                .window = BT_GAP_SCAN_FAST_WINDOW,
        };

        // Start scanning
        err = bt_le_scan_start(&scan_param, device_found); 
        if (err){ 
                LOG_ERR("Scanning failed to start (err %d)", err);
                return 0;
        }

        LOG_INF("Scanning started");

        // No main loop everything happens in callback

        while(1){
                k_sleep(K_SECONDS(1)); 
        }
}
