#pragma once
#include <esp_err.h>
#include <driver/ledc.h>

// The display is configured directly through ESP-IDF for active-low output.
// Arduino 3 ledcWrite takes a pin, not a channel, and cannot manage these
// unregistered channels. Keep setup and duty updates in the same IDF API.
inline void writePwmDuty(ledc_mode_t mode, ledc_channel_t channel, uint32_t duty) {
  ESP_ERROR_CHECK(ledc_set_duty(mode, channel, duty));
  ESP_ERROR_CHECK(ledc_update_duty(mode, channel));
}

inline esp_err_t setupKeyboardPwm(int pin) {
  ledc_timer_config_t timer = {};
  timer.speed_mode = LEDC_LOW_SPEED_MODE;
  timer.duty_resolution = LEDC_TIMER_8_BIT;
  timer.timer_num = LEDC_TIMER_3;
  timer.freq_hz = 5000;
  timer.clk_cfg = LEDC_USE_APB_CLK;
  esp_err_t err = ledc_timer_config(&timer);
  if (err != ESP_OK) return err;

  ledc_channel_config_t channel = {};
  channel.gpio_num = pin;
  channel.speed_mode = LEDC_LOW_SPEED_MODE;
  channel.channel = LEDC_CHANNEL_6;
  channel.intr_type = LEDC_INTR_DISABLE;
  channel.timer_sel = LEDC_TIMER_3;
  channel.duty = 0;
  return ledc_channel_config(&channel);
}
