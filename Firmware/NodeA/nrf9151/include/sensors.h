#ifndef SENSORS_H
#define SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

int sensors_init(void);
int sensors_sample_once(void);

#ifdef __cplusplus
}
#endif

#endif // SENSORS_H
