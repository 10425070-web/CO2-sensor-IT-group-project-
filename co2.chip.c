#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  pin_t pin_out;
  timer_t timer;
  uint32_t attr_co2ppm;
} chip_state_t;

static void update_output(void *user_data) {
  chip_state_t *chip = (chip_state_t *)user_data;

  int ppm = attr_read(chip->attr_co2ppm);

  // map 400..5000 ppm -> 0..5V
  float voltage = ((float)(ppm - 0) / (1600.0f - 0.0f)) * 5.0f;
  if (voltage < 0.0f) voltage = 0.0f;
  if (voltage > 5.0f) voltage = 5.0f;

  pin_dac_write(chip->pin_out, voltage);
}

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  memset(chip, 0, sizeof(chip_state_t));

  chip->pin_out = pin_init("OUT", ANALOG);
  chip->attr_co2ppm = attr_init("co2ppm", 0);

  const timer_config_t timer_config = {
    .callback = update_output,
    .user_data = chip,
  };

  chip->timer = timer_init(&timer_config);

  update_output(chip);                 // push initial value immediately
  timer_start(chip->timer, 100000, true); // refresh every 100 ms
}
