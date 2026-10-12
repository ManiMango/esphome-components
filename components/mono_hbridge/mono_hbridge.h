#pragma once

#include <cmath>

#include "esphome/core/application.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/output/float_output.h"

namespace esphome::mono_hbridge {

/// Drives a strip of alternating-polarity LEDs from two outputs wired as an H-bridge.
///
/// The two polarities are lit in two separate halves of every PWM period, so both can be fully on at once.
/// Output A is high for the first half of the period. Output B must run on the same timer, shifted by half a period
/// (phase_angle: 180°), so it is high for the second half. A polarity is lit only while its own output is high and
/// the other is low.
class MonoHBridgeOutput final : public light::LightOutput {
 public:
  void set_pin_a(output::FloatOutput *pin_a) { this->pin_a_ = pin_a; }
  void set_pin_b(output::FloatOutput *pin_b) { this->pin_b_ = pin_b; }

  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
    return traits;
  }

  void setup_state(light::LightState *state) override { this->state_ = state; }

  void write_state(light::LightState *state) override {
    float brightness;
    state->current_values_as_brightness(&brightness);
    this->write_levels_(brightness * this->level_a_, brightness * this->level_b_);
  }

  /// Scale the brightness of each polarity (0..1). Used by effects; (1, 1) is the solid mode.
  void set_pattern(float level_a, float level_b) {
    this->level_a_ = level_a;
    this->level_b_ = level_b;
    if (this->state_ != nullptr)
      this->write_state(this->state_);
  }

 protected:
  void write_levels_(float a, float b) {
    this->pin_a_->set_level(a * 0.5f);
    this->pin_b_->set_level(b * 0.5f);
  }

  output::FloatOutput *pin_a_{nullptr};
  output::FloatOutput *pin_b_{nullptr};
  light::LightState *state_{nullptr};
  float level_a_{1.0f};
  float level_b_{1.0f};
};

enum class PolarityPattern : uint8_t {
  POLARITY_PATTERN_ALTERNATE,
  POLARITY_PATTERN_CROSSFADE,
  POLARITY_PATTERN_BLINK,
  POLARITY_PATTERN_BREATHE,
  POLARITY_PATTERN_STROBE_ALTERNATE,
  POLARITY_PATTERN_BLINK_A,
  POLARITY_PATTERN_BLINK_B,
  POLARITY_PATTERN_ONLY_A,
  POLARITY_PATTERN_ONLY_B,
};

/// An effect that varies the two polarities over time. The brightness slider still sets the overall level.
class PolarityEffect : public light::LightEffect {
 public:
  PolarityEffect(const char *name, PolarityPattern pattern) : LightEffect(name), pattern_(pattern) {}

  void set_period(uint32_t period) { this->period_ = period; }

  void start() override { this->start_time_ = App.get_loop_component_start_time(); }

  void stop() override { this->output_()->set_pattern(1.0f, 1.0f); }

  void apply() override {
    const uint32_t elapsed = (App.get_loop_component_start_time() - this->start_time_) % this->period_;
    const float phase = static_cast<float>(elapsed) / static_cast<float>(this->period_);
    float a = 0.0f;
    float b = 0.0f;
    switch (this->pattern_) {
      case PolarityPattern::POLARITY_PATTERN_ALTERNATE:
        a = phase < 0.5f ? 1.0f : 0.0f;
        b = 1.0f - a;
        break;
      case PolarityPattern::POLARITY_PATTERN_CROSSFADE:
        b = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * phase);
        a = 1.0f - b;
        break;
      case PolarityPattern::POLARITY_PATTERN_BLINK:
        a = b = phase < 0.5f ? 1.0f : 0.0f;
        break;
      case PolarityPattern::POLARITY_PATTERN_BREATHE:
        a = b = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * phase);
        break;
      case PolarityPattern::POLARITY_PATTERN_STROBE_ALTERNATE:
        a = phase < 0.25f ? 1.0f : 0.0f;
        b = (phase >= 0.5f && phase < 0.75f) ? 1.0f : 0.0f;
        break;
      case PolarityPattern::POLARITY_PATTERN_BLINK_A:
        a = phase < 0.5f ? 1.0f : 0.0f;
        break;
      case PolarityPattern::POLARITY_PATTERN_BLINK_B:
        b = phase < 0.5f ? 1.0f : 0.0f;
        break;
      case PolarityPattern::POLARITY_PATTERN_ONLY_A:
        a = 1.0f;
        break;
      case PolarityPattern::POLARITY_PATTERN_ONLY_B:
        b = 1.0f;
        break;
    }
    this->output_()->set_pattern(a, b);
  }

 protected:
  MonoHBridgeOutput *output_() const { return static_cast<MonoHBridgeOutput *>(this->state_->get_output()); }

  PolarityPattern pattern_;
  uint32_t period_{2000};
  uint32_t start_time_{0};
};

}  // namespace esphome::mono_hbridge
