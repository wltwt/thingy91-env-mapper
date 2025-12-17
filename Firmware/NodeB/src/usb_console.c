#include "usb_console.h"
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

void usb_console_init(uint32_t timeout_ms)
{
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;
    uint32_t t0 = k_uptime_get();

    usb_enable(NULL);

    if (!device_is_ready(dev)) {
        return;
    }

    while (!dtr && (k_uptime_get() - t0 < timeout_ms)) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(50));
    }
}
