#include "usb_console.h"
#include "ble_scan.h"
#include "sample.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

int main(void)
{
    usb_console_init(3000);

    printk("Node B starting\n");

    if (bt_enable(NULL)) {
        printk("Bluetooth init failed\n");
        return 0;
    }
    
    printk("Bluetooth enabled\n");
    printk("Starting BLE scan\n");

    printk("sample_worker started\n");

    sample_init();

    if (ble_scan_start()) {
        printk("Scan start failed\n");
        return 0;
    }

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
