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


// For UART
#include <zephyr/sys/util.h>     // CONTAINER_OF
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>


#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

#define PKT_MAGIC 0xA5
#define PKT_VER   1

struct __packed env_pkt_wire {
    uint8_t  magic;        // 0xA5
    uint8_t  ver;          // 1
    uint16_t seq_le;       // little-endian
    int16_t  t_c_e2_le;    // temp * 100
    uint16_t rh_e2_le;     // RH * 100
    int32_t  lat_e7_le;    // lat * 1e7
    int32_t  lon_e7_le;    // lon * 1e7
    uint8_t  src;          // team id
    uint8_t  crc8;         // over bytes [magic..src]
};

BUILD_ASSERT(sizeof(struct env_pkt_wire) == 1+1+2+2+2+4+4+1+1, "pkt size changed");






/* Nordic company-ID */
#define COMPANY_ID_CODE 0x0059


/* Define advertisement protocol structure */
typedef struct __packed adv_mfg_data {
	uint16_t company_code; 	/* Company Identifier Code. */
	uint8_t magic; 			/* Ensure correct protocol type */
	uint8_t ver; 			/* Protocol Version*/
	uint16_t seq; 			/* Sequence number */
	int16_t t_c_e2; 		/* Temperature (Celsius) */
	uint16_t rh_e2; 		/* Humidity */
	int32_t lat_e7; 		/* Latitude */
	int32_t lon_e7; 		/* Longitude */
	uint8_t src; 			/* Team number */
	// uint8_t crc8; /* Maybe crc */
} adv_mfg_data_type;

#define USER_BUTTON DK_BTN1_MSK


/* Create an LE Advertising Parameters variable */
static const struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE, /* No options specified */
			3200, /* Min Advertising Interval 2000ms (3200*0.625ms) */
			8000, /* Max Advertising Interval 5000ms (8000*0.625ms) */
			NULL); /* Set to NULL for undirected advertising */



static adv_mfg_data_type adv_payload = { COMPANY_ID_CODE, 0x00 };

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
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_payload, sizeof(adv_payload)),
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

#define UART_BUF_SIZE 128
#define UART_WAIT_FOR_RX 50000
#define RX_THREAD_STACK 1024

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0)); // eller uart1



/* STEP 6.1 - Declare the FIFOs */
static K_FIFO_DEFINE(fifo_uart_rx_data);


static struct k_work_delayable uart_work;


#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)

struct uart_data_t {
    void *fifo_reserved;
    uint8_t data[UART_BUF_SIZE];
    uint16_t len;
};


static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    struct uart_data_t *buf;

    switch (evt->type) {

    case UART_RX_RDY:
        //printk("EVT RX_RDY\n");

        buf = CONTAINER_OF(evt->data.rx.buf, struct uart_data_t, data[0]);
        buf->len += evt->data.rx.len;
		//printk("RX_RDY len=%u\n", buf->len);
        break;

    case UART_RX_DISABLED:
		//printk("EVT RX_DISABLED\n");
        buf = k_malloc(sizeof(*buf));
        if (!buf) {
            k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
            return;
        }
        buf->len = 0;
        uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);
        break;

    case UART_RX_BUF_REQUEST:
	    //printk("EVT BUF_REQUEST\n");
        buf = k_malloc(sizeof(*buf));
        if (!buf) {
            return;
        }
        buf->len = 0;
        uart_rx_buf_rsp(uart, buf->data, sizeof(buf->data));
        break;

    case UART_RX_BUF_RELEASED:
        //printk("EVT BUF_RELEASED\n");
        buf = CONTAINER_OF(evt->data.rx_buf.buf, struct uart_data_t, data[0]);
        if (buf->len > 0) {
            k_fifo_put(&fifo_uart_rx_data, buf);
        } else {
            k_free(buf);
        }
        break;

    default:
        break;
    }
}

static void uart_work_handler(struct k_work *item)
{
	struct uart_data_t *buf;

	buf = k_malloc(sizeof(*buf));
	if (buf) {
		buf->len = 0;
	} else {
		LOG_WRN("Not able to allocate UART receive buffer");
		k_work_reschedule(&uart_work, UART_WAIT_FOR_BUF_DELAY);
		return;
	}

	uart_rx_enable(uart, buf->data, sizeof(buf->data), UART_WAIT_FOR_RX);
}

static uint8_t acc[256];
static size_t acc_len;

static uint8_t crc8_atm(const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}



static bool try_extract_one(struct env_pkt_wire *out)
{
    // finn magic
    size_t i = 0;
    while (i < acc_len && acc[i] != PKT_MAGIC) i++;

    if (i > 0) {
        memmove(acc, acc + i, acc_len - i);
        acc_len -= i;
    }

    if (acc_len < sizeof(struct env_pkt_wire)) return false;

    struct env_pkt_wire p;
    memcpy(&p, acc, sizeof(p));

    // basic checks
    if (p.magic != PKT_MAGIC || p.ver != PKT_VER) {
        // dropp 1 byte og prøv igjen senere
        memmove(acc, acc + 1, acc_len - 1);
        acc_len -= 1;
        return false;
    }

    uint8_t crc = crc8_atm((const uint8_t*)&p, offsetof(struct env_pkt_wire, crc8));
    if (crc != p.crc8) {
        // CRC feil: dropp magic-byten og resync
        memmove(acc, acc + 1, acc_len - 1);
        acc_len -= 1;
        return false;
    }

    // valid: returner den og fjern fra buffer
    *out = p;
    memmove(acc, acc + sizeof(p), acc_len - sizeof(p));
    acc_len -= sizeof(p);
    return true;
}

#define LINE_MAX 128
static char line[LINE_MAX];
static size_t line_len;

static void update_adv_from_uart_pkt(const struct env_pkt_wire *p)
{
    adv_payload.company_code = COMPANY_ID_CODE;
    adv_payload.magic = p->magic;
    adv_payload.ver   = p->ver;

    adv_payload.seq    = sys_le16_to_cpu(p->seq_le);
    adv_payload.t_c_e2 = (int16_t)sys_le16_to_cpu(p->t_c_e2_le);
    adv_payload.rh_e2  = sys_le16_to_cpu(p->rh_e2_le);
    adv_payload.lat_e7 = (int32_t)sys_le32_to_cpu(p->lat_e7_le);
    adv_payload.lon_e7 = (int32_t)sys_le32_to_cpu(p->lon_e7_le);
    adv_payload.src    = p->src;
}


static void uart_rx_thread(void)
{
    for (;;) {
        struct uart_data_t *buf = k_fifo_get(&fifo_uart_rx_data, K_FOREVER);

        // append
        size_t copy = MIN(buf->len, sizeof(acc) - acc_len);
        memcpy(acc + acc_len, buf->data, copy);
        acc_len += copy;

        // prøv å trekke ut 0..N pakker
        struct env_pkt_wire p;
        while (try_extract_one(&p)) {
			update_adv_from_uart_pkt(&p);
			int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
			if (err) {
				printk("adv_update err=%d\n", err);
			}
			/*
            uint16_t seq = sys_le16_to_cpu(p.seq_le);
            int16_t  t   = (int16_t)sys_le16_to_cpu(p.t_c_e2_le);
            uint16_t rh  = sys_le16_to_cpu(p.rh_e2_le);
            int32_t  lat = (int32_t)sys_le32_to_cpu(p.lat_e7_le);
            int32_t  lon = (int32_t)sys_le32_to_cpu(p.lon_e7_le);

            printk("PKT seq=%u t=%d rh=%u lat=%d lon=%d src=%u\n",
                   seq, t, rh, lat, lon, p.src);

            // TODO: oppdater adv payload her
			update_adv_from_uart_pkt(&p);

			            // 2) push til BLE advertising
            int err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
            if (err) {
                printk("adv_update err=%d\n", err);
            }

            // (valgfritt) debug
            uint16_t seq = sys_le16_to_cpu(p.seq_le);
			
            printk("Updated ADV from seq=%u\n", seq);
			*/
        }

        k_free(buf);
    }
}


K_THREAD_DEFINE(uart_rx_thread_id, RX_THREAD_STACK, uart_rx_thread,
                NULL, NULL, NULL, 7, 0, 0);



int main(void)
{
	int blink_status = 0;
	int seq_incr = 0;
	int err;

	//LOG_INF("Starting NRF5340 \n");

	printk("nRF5340 UART RX app started\n");

	if (!device_is_ready(uart)) {
        printk("UART not ready\n");
        return -1;
    }


	k_work_init_delayable(&uart_work, uart_work_handler);

	int ret;

	ret = uart_callback_set(uart, uart_cb, NULL);
	printk("uart_callback_set ret=%d\n", ret);

	struct uart_data_t *rx = k_malloc(sizeof(*rx));
	if (!rx) { printk("OOM rx\n"); return -1; }
	rx->len = 0;

	ret = uart_rx_enable(uart, rx->data, sizeof(rx->data), UART_WAIT_FOR_RX);
	printk("uart_rx_enable ret=%d\n", ret);


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
	while (1) {
    	k_sleep(K_SECONDS(1));

		//printk("Sleeping");
	}

	/*
	char line[64];
	int i = 0;

	while (1) {
		uint8_t c;
		while (uart_poll_in(uart, &c) == 0) {
			if (c == '\n' || i == (sizeof(line)-1)) {
				line[i] = 0;
				printk("RX: %s\n", line);
				i = 0;
			} else if (c != '\r') {
				line[i++] = (char)c;
			}
		}
		k_sleep(K_MSEC(1));
	}
	*/

	/*
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
	*/
}