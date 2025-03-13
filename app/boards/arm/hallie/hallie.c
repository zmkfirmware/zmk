#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

/* Current cell voltage in units of 1.25/16mV */
uint16_t voltage;
/* Average current in units of 1.5625uV / Rsense */
int16_t avg_current;
/* Remaining capacity as a %age */
uint16_t state_of_charge;
/* Internal temperature in units of 1/256 degrees C */
int16_t internal_temp;
/* Full charge capacity in 5/Rsense uA */
uint16_t full_cap;
/* Remaining capacity in 5/Rsense uA */
uint16_t remaining_cap;
/* Time to empty in seconds */
uint16_t time_to_empty;
/* Time to full in seconds */
uint16_t time_to_full;
/* Cycle count in 1/100ths (number of charge/discharge cycles) */
uint16_t cycle_count;
/* Design capacity in 5/Rsense uA */
uint16_t design_cap;