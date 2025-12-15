#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtt_test, LOG_LEVEL_INF);

int main(void)
{
    printk("RTT TEST: printk works\n");
    LOG_INF("RTT TEST: LOG_INF works");
    LOG_ERR("RTT TEST: LOG_ERR works");

    while (1) {
        LOG_INF("RTT heartbeat...");
        k_sleep(K_SECONDS(1));
    }
}
