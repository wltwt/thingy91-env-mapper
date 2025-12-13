#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

int sensors_init(void)
{
    LOG_INF("sensors_init() called");
    return 0;
}

int sensors_sample_once(void)
{
    static int counter;
    counter++;
    LOG_INF("sensors_sample_once(): counter=%d", counter);
    return counter;
}
