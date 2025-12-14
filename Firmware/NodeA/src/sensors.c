/*
sensors.c

Get temperature values and such

INFO: 
https://docs.zephyrproject.org/latest/hardware/peripherals/sensor/fetch_and_get.html#sensor-fetch-and-get
*/



#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include "sensors.h"


#define BME_NODE DT_NODELABEL(bme680)
static const struct device *bme = DEVICE_DT_GET(BME_NODE);

// Get I2C address of environment chip
//#define I2C_NODE DT_NODELABEL(bme680)

// BME680/688 read registers
//#define BME680_REG_CHIP_ID 0xD0
//#define BME680_CHIP_ID     0x61

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);
//static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C_NODE);

int ret = 0;

int sensors_init(void)
{
    /*
    if (!device_is_ready(dev_i2c.bus)) {
	    LOG_INF("I2C bus %s is not ready!\n\r",dev_i2c.bus->name);
	    return -1;
    }
    */

    if (!device_is_ready(bme)) {
        printk("BME68x device not ready\n");
        return 0;
    }
    
    LOG_INF("sensors_init() called");
    return 0;
}

int sensors_sample_once(void)
{
    /*
    uint8_t bme_id = 0;
    ret = i2c_reg_read_byte_dt(&dev_i2c, BME680_REG_CHIP_ID, &bme_id);
    if (ret != 0) {
        LOG_ERR("chip-id read failed: %d", ret);
    }
    */

    struct sensor_value t, p, h;

    sensor_sample_fetch(bme);
    sensor_channel_get(bme, SENSOR_CHAN_AMBIENT_TEMP, &t);
    sensor_channel_get(bme, SENSOR_CHAN_PRESS, &p);
    sensor_channel_get(bme, SENSOR_CHAN_HUMIDITY, &h);
    static int counter;
    counter++;
    LOG_INF("sensors_sample_once(): counter=%d", counter);

    //LOG_INF("BME chip-id = 0x%02x", bme_id);

    LOG_INF("T=%d.%06d C  P=%d.%06d kPa  H=%d.%06d %%\n",
            t.val1, t.val2, p.val1, p.val2, h.val1, h.val2);

    //LOG_INF("I2C bus=%s addr=0x%02x", dev_i2c.bus->name, dev_i2c.addr);
    return counter;
}
