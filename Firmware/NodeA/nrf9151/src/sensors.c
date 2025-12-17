/*
* sensors.c
* 
* Get temperature values and such.
*
* Code is heavily referenced from nordic's courses 
* also example code shown in link below
* 
* https://docs.zephyrproject.org/latest/hardware/peripherals/sensor/fetch_and_get.html#sensor-fetch-and-get
* 
*/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

#include "sensors.h"

/* Get bme680  */
#define BME_NODE DT_NODELABEL(bme680)

/* Initialize logging module for sensors */
LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

/* Get pointer for bme68x-sensor */
static const struct device *bme = DEVICE_DT_GET(BME_NODE);

/* Initialize env-sample package */
static struct env_sample env_payload;


/* Convert floating point to integer 
*  for avoiding trouble in ble packet transfer
*/
static int16_t temperature_conv(const struct sensor_value *v)
{
    return (int16_t)(v->val1 * 100 + v->val2 / 10000);
}

/* Same purpose as above function */
static uint16_t humidity_conv(const struct sensor_value *v)
{
    int32_t x = v->val1 * 100 + v->val2 / 10000;
    return (uint16_t)x;
}



int sensors_init(void)
{

    if (!device_is_ready(bme)) {
        LOG_INF("BME68x device not ready\n");
        return 0;
    }
    
    LOG_INF("sensors_init() called");
    return 0;
}



int update_env_sample(void)
{
    struct sensor_value t, h;

    int err = sensor_sample_fetch(bme);
    if (err != 0) {
        return err;
    }

    /* Put temperature values in local payload */
    sensor_channel_get(bme, SENSOR_CHAN_AMBIENT_TEMP, &t);
    env_payload.t_c_e2 = temperature_conv(&t);

    /* Put realtive humidity in local payload */
    sensor_channel_get(bme, SENSOR_CHAN_HUMIDITY, &h);
    env_payload.rh_e2 = humidity_conv(&h);

    //LOG_INF("T=%d C, H=%d\n", env_payload.t_c_e2, env_payload.rh_e2);
    return 0;
}

int get_env(struct env_sample *pkt) {
    update_env_sample();
    *pkt = env_payload;
    return 0;
}
