# mono_hbridge

Light platform for a strip of alternating-polarity LEDs (all one color) driven by two PWM outputs
wired as an H-bridge. It appears in Home Assistant as a dimmable light with these effects:

| Effect | What it does |
| --- | --- |
| None | Solid: all LEDs at the brightness slider level |
| Alternate | Hard switch between the two polarities |
| Cross Fade | Smooth fade from one polarity to the other and back |
| Blink | All LEDs on and off together |
| Breathe | All LEDs fade in and out together |
| Strobe Alternate | Quick flashes, one polarity then the other |
| Blink A / Blink B | Only one polarity blinks |
| Polarity A Only / Polarity B Only | Only one polarity lit |

Each effect has an optional `period` (the time of one full cycle).

```yaml
output:
  - platform: ledc
    id: hbridge_a
    pin: GPIO3
    frequency: 1220Hz
    channel: 0
  - platform: ledc
    id: hbridge_b
    pin: GPIO4
    frequency: 1220Hz
    channel: 1 # same timer as hbridge_a (channels 0/1, 2/3, ...)
    phase_angle: 180° # required

light:
  - platform: mono_hbridge
    name: "LED Strip"
    pin_a: hbridge_a
    pin_b: hbridge_b
    # effects: omit to get all of the above, or list your own, e.g.
    # effects:
    #   - polarity_crossfade:
    #       period: 2s
```

Output B must be on the same LEDC timer as output A (neighbouring channels 0/1, 2/3, ...) and use
`phase_angle: 180°`. Then A is high for the first half of each PWM period and B for the second half, so each
polarity is lit in its own half and can be fully on at the same time. Without the phase shift both pins
switch together and only the random start offset between timers lights the strip.
