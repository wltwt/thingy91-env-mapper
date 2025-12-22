#include "usb_console.h"
#include "ble_scan.h"
#include "sample.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>


static void bt_ready(int err)
{
    if (err) {
        printk("Bluetooth init failed (%d)\n", err);
        return;
    }

    printk("Bluetooth ready\n");

    sample_init();

    if (ble_scan_start()) {
        printk("Scan start failed\n");
        return;
    }

    printk("Scanning started\n");
}



int main(void)
{
    usb_console_init(3000);
    printk("Node B starting\n");

    int err = bt_enable(bt_ready);
    if (err) {
        printk("bt_enable failed (%d)\n", err);
        return 0;
    }

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}