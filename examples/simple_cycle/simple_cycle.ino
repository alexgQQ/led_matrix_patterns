/*
Example showcase of led bar colors and animations. Simply cycles through each indefinitley.
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

uint32_t pattern_time = millis();
uint32_t color_time = millis();

void setup() {
  bars.begin();
}

void loop() {
  if ((millis() - color_time) > 1000) {
    bars.next_color();
    color_time = millis();
  }
  if ((millis() - pattern_time) > 5000) {
    bars.next_pattern();
    pattern_time = millis();
  }
  bars.render();
}
