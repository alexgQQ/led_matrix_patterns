/*
Example test program to see a single animation. Helpfull for quick testing with development.
Intended to be used with a Arduino Pro or Pro-Mini. The output needs to be a PWM compatiblke digital pin,
by default this is digital pin 5. Connect this pin, along with power and ground, to the led strip input.
*/

#include <led_bars.h>

#define LED_DATA_PIN 5
#define LED_SEGMENTS 4
#define LED_PER_SEGMENT 60

/*
TODO: The way the segments are wired caused the first led on the first strip to 
actually be an inner segment. So from right to left the segments are off. Ideally
the segments are wired from left to right in order. For now these are hardcoded for that reason
but would be good to fix the wiring and potentially construct this automatically.
*/
segment segments[LED_SEGMENTS] = {
  [0] = { .first_position = 239, .reverse = true },
  [1] = { .first_position = 120, .reverse = false },
  [2] = { .first_position = 0, .reverse = false },
  [3] = { .first_position = 119, .reverse = true },
};

LED_Bars bars(LED_SEGMENTS, LED_PER_SEGMENT, LED_DATA_PIN, segments);

void setup() {
  bars.begin();
  bars.set_pattern(&bars.falling_drift_sparkle_waves);
  bars.set_color(&bars.blue_magenta_blue);
}

void loop() {
  bars.render();
}
