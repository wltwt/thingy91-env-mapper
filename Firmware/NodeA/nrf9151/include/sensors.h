#ifndef SENSORS_H
#define SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

struct env_sample {
    int16_t  t_c_e2;
    uint16_t rh_e2;
};


int sensors_init(void);
int update_env_sample(void);
int get_env(struct env_sample *pkt);

#ifdef __cplusplus
}
#endif

#endif // SENSORS_H
