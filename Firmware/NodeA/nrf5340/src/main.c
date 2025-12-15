/*
* main.c
* 
* Code is referenced heavily from various nordic course example code.
*
*/

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

// Bluetooth
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>

#include <zephyr/sys/byteorder.h>

#include <dk_buttons_and_leds.h>


/* Nordic company-ID */
#define COMPANY_ID_CODE 0x0059


/* Define advertisement protocol structure */
typedef struct __packed adv_mfg_data {
	uint16_t company_code; /* Company Identifier Code. */
	uint8_t magic; /* Ensure correct protocol type */
	uint8_t ver; /* Protocol Version*/
	uint16_t seq; /* Sequence number */
	int16_t t_c_e2; /* Temperature (Celsius) */
	uint16_t rh_e2; /* Humidity */
	int32_t lat_e7; /* Latitude */
	int32_t lon_e7; /* Longitude */
	uint8_t src; /* Team number */
	// uint8_t crc8; /* Maybe crc */
} adv_mfg_data_type;

#define USER_BUTTON DK_BTN1_MSK


/* Create an LE Advertising Parameters variable */
static const struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, /* No options specified */
			3200, /* Min Advertising Interval 2000ms (3200*0.625ms) */
			8000, /* Max Advertising Interval 5000ms (8000*0.625ms) */
			NULL); /* Set to NULL for undirected advertising */



static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, 0x00 };

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define SEND_INTERVAL 2000

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL SEND_INTERVAL

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	/* STEP 3 - Include the Manufacturer Specific Data in the advertising packet. */
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};


/* STEP 4.2.2 - Declare the URL data to include in the scan response */
/*
static unsigned char url_data[] = { 0x17, '/', '/', 'a', 'c', 'a', 'd', 'e', 'm',
				    'y',  '.', 'n', 'o', 'r', 'd', 'i', 'c', 's',
				    'e',  'm', 'i', '.', 'c', 'o', 'm' };
*/

/*
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_URI, url_data, sizeof(url_data)),
};
*/

/* STEP 5 - Add the definition of callback function and update the advertising data dynamically */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & button_state & USER_BUTTON) {
		adv_mfg_data.t_c_e2 = sys_cpu_to_le16(int16_t) + 1;
		bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
	}
}


/* STEP 4.1 - Define the initialization function of the buttons and setup interrupt.  */
static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}



int main(void)
{
	int blink_status = 0;
	int seq_incr = 0;
	int err;

	LOG_INF("Starting Lesson 2 - Exercise 2 \n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return -1;
	}
	/* STEP 4.2 - Setup buttons on your board  */
	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return -1;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized\n");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return -1;
	}

	LOG_INF("Advertising successfully started\n");

	for (;;) {
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		seq_incr++;
		adv_mfg_data.seq = (uint16_t)seq_incr;

		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
		if (err) {
			LOG_ERR("Failed update of advertisment data");
		}

		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));

	}
}