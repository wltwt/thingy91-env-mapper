#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include "sensors.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("App boot");

    int err = sensors_init();
    LOG_INF("sensors_init returned %d", err);

    while (1) {
        int val = sensors_sample_once();
        LOG_INF("main got %d", val);
        k_sleep(K_SECONDS(2));
    }
}
