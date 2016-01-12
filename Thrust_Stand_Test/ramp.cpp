#include "Energia.h"
#include "ramp.h"

#define min(x,y) (x < y ? x : y)
#define max(x,y) (x > y ? x : y)


void ramp_init(ramp *r) {
  r->len = 0;
}

int ramp_add_range(ramp *r, int start_pwm, int end_pwm, int ramp_time_ms) {
  ramp_item *curr, *prev;
  if (r->len == 32) {
    return 0;
  }
  curr = &r->items[r->len++];
  prev = curr-1;
  if (r->len == 1) {
    curr->start_time_us = 0;
  } 
  else {
    curr->start_time_us = prev->start_time_us + prev->ramp_time_us;
  }
  curr->start_pwm = start_pwm;
  curr->end_pwm = end_pwm;
  curr->ramp_time_us = ramp_time_ms*1000;

  return 1;
}

int ramp_add_static(ramp *r, int pwm, int hold_time_ms) {
  return ramp_add_range(r, pwm, pwm, hold_time_ms);
}


int ramp_get_pwm(ramp *r, uint64_t curr_time) {
  ramp_item *p = r->items;
  int i, pwm, pwm_diff;
  int64_t time_diff;

  pwm = -1;
  for (i = 0; i < r->len; i++) {
    if (curr_time > p[i].start_time_us + p[i].ramp_time_us) {
      continue;
    }

    //if it's just a static pwm hold, we don't need to do anything
    if (p[i].start_pwm == p[i].end_pwm) {
      //static thrust
      pwm = p[i].start_pwm;
    } 
    else {
      //figure out where we are in the ramp up
      time_diff = curr_time - p[i].start_time_us;
      pwm_diff = p[i].end_pwm - p[i].start_pwm;
      pwm = p[i].start_pwm + ((time_diff * pwm_diff) / p[i].ramp_time_us);
    }
    break;
  }
  return pwm;
}

