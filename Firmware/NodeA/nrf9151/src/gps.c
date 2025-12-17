/*
gps.c


INFO:
https://academy.nordicsemi.com/courses/cellular-iot-fundamentals/lessons/lesson-6-cellular-fundamentals/topic/lesson-6-gnss/
*/
#include <ncs_version.h>



#include <stdio.h>
#include <ncs_version.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>

/* STEP 4 - Include the header file for the GNSS interface */
#include <nrf_modem_gnss.h>

#include "gps.h"


LOG_MODULE_REGISTER(gps, LOG_LEVEL_INF);


static struct nrf_modem_gnss_pvt_data_frame pvt_data;


static int64_t gnss_start_time;


static bool first_fix = false;
static bool initialized = false;


static struct gps_sample payload;


static K_SEM_DEFINE(lte_connected, 0, 1);

static int32_t deg_to_e7(double deg)
{
    // robust nok til dette bruket
    return (int32_t)(deg * 10000000.0);
}

static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}

	return 0;
}

static void set_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	payload.lat_e7 = deg_to_e7(pvt_data->latitude);
	payload.lon_e7 = deg_to_e7(pvt_data->longitude);

    LOG_INF("Found latitude: %d, longitude: %d", payload.lat_e7, payload.lon_e7);

	/*
	LOG_INF("Latitude:       %.06f", pvt_data->latitude);
	LOG_INF("Longitude:      %.06f", pvt_data->longitude);
	LOG_INF("Altitude:       %.01f m", (double)pvt_data->altitude);
	LOG_INF("Time (UTC):     %02u:%02u:%02u.%03u",
	       pvt_data->datetime.hour,
	       pvt_data->datetime.minute,
	       pvt_data->datetime.seconds,
	       pvt_data->datetime.ms);

	*/
}

static void gnss_event_handler(int event)
{
    int err;

    switch (event) {
        case NRF_MODEM_GNSS_EVT_PVT:
            err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
            if (err) {
                LOG_ERR("nrf_modem_gnss_read failed, err %d", err);
                return;
            }

            // (valgfritt) satellittlogging, nå med fersk pvt_data
            int num_satellites = 0;
            for (int i = 0; i < 12; i++) {
                if (pvt_data.sv[i].signal != 0) {
                    num_satellites++;
                }
            }
            static int64_t last_log_ms;

            if (k_uptime_get() - last_log_ms > 1000) {
                last_log_ms = k_uptime_get();

                int32_t lat_e7 = (int32_t)(pvt_data.latitude  * 10000000.0);
                int32_t lon_e7 = (int32_t)(pvt_data.longitude * 10000000.0);

                LOG_INF("PVT flags=0x%08x sats=%d lat_e7=%d lon_e7=%d",
                    pvt_data.flags, num_satellites, lat_e7, lon_e7);
            }

            if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
                payload.fix_valid = true;
                payload.ts_ms = k_uptime_get();
                set_fix_data(&pvt_data);

                if (!first_fix) {
                    LOG_INF("Time to first fix: %lld s",
                            (k_uptime_get() - gnss_start_time) / 1000);
                    first_fix = true;
                }
            } else {
                payload.fix_valid = false;
                payload.ts_ms = k_uptime_get();
                payload.lat_e7 = 0;
                payload.lon_e7 = 0;
            }
            break;

        case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
            LOG_INF("GNSS has woken up");
            break;

        case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
            LOG_INF("GNSS enter sleep after fix");
            break;

        default:
            break;
    }
}


int gps_init(void)
{
    int err;

    err = modem_configure();
    if (err) return err;

    err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
    if (err) return err;

    err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
    if (err) return err;


    
    err = nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_PERIODIC_INTERVAL);
    if (err) LOG_WRN("fix_interval_set err=%d", err);

    err = nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_PERIODIC_TIMEOUT);
    if (err) LOG_WRN("fix_retry_set err=%d", err);

    

    //nrf_modem_gnss_fix_interval_set(1);  // 0 = continuous
    //nrf_modem_gnss_fix_retry_set(180);   // 180s (eller høyere)


    gnss_start_time = k_uptime_get();

    LOG_INF("Starting GNSS");
    err = nrf_modem_gnss_start();
    if (err) return err;

    payload.fix_valid = false;
    payload.lat_e7 = 0;
    payload.lon_e7 = 0;
    payload.ts_ms = k_uptime_get();
    initialized = true;

    return 0;
}


int get_gps_payload(struct gps_sample *pkt) {
	if (!initialized) return -EINVAL;
	*pkt = payload;
	return 0;
}