#ifndef __RAMP_H__
#define __RAMP_H__

typedef struct ramp_item {
    uint64_t start_time_us;
    int start_pwm;
    int end_pwm;
    int ramp_time_us;
} ramp_item;

typedef struct ramp {
    int len;
    ramp_item items[64];
} ramp;

void ramp_init(ramp *r);
int ramp_add_range(ramp *r, int start_pwm, int end_pwm, int ramp_time_ms);
int ramp_add_static(ramp *r, int pwm, int hold_time_ms);
int64_t ramp_get_pwm(ramp *r, uint64_t curr_time);

#endif
