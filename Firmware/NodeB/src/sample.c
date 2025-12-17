#include "sample.h"
#include <zephyr/kernel.h>
#include <stdio.h>

K_MSGQ_DEFINE(sample_q, sizeof(struct node_b_sample), 8, 8);

static void sample_to_json(const struct node_b_sample *s,
                           char *buf, size_t len)
{
    snprintf(buf, len,
        "{\"ts\":%u,\"seq\":%u,\"t_c_e2\":%d,\"rh_e2\":%u,"
        "\"lat_e7\":%d,\"lon_e7\":%d,\"src\":%u,\"rssi\":%d}",
        (uint32_t)s->ts_ms, s->seq, s->t_c_e2, s->rh_e2,
        s->lat_e7, s->lon_e7, s->src, s->rssi);
}

static void sample_worker(void)
{
    struct node_b_sample s;
    char json[192];

    while (1) {
        k_msgq_get(&sample_q, &s, K_FOREVER);
        sample_to_json(&s, json, sizeof(json));
        printk("JSON %s\n", json);
    }
}

K_THREAD_DEFINE(sample_thread, 2048, sample_worker,
                NULL, NULL, NULL, 5, 0, 0);

void sample_push(const struct node_b_sample *s)
{
    printk("sample_push called\n");
    k_msgq_put(&sample_q, s, K_NO_WAIT);
}

void sample_init(void)
{
    /* Tom nå – men nyttig senere */
}
