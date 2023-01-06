/*
Example for software controls for a set of led bars attached to a rotary encoder.
Intended to be used with a Arduino Pro or Pro-Mini and requires the `Encoder` library.

The inputs expect a rotary encoder with its two channels connected to external interrupts
and an external input that will be polled for button presses. By default this uses digital pins
2, 3 and 4 respectivley. The output needs to be a PWM compatiblke digital pin, by default this is 
digital pin 5. Connect this pin, along with power and ground, to the led strip input.

Usage:
- Press the rotary encoder to transition states in this order:
  - off
  - on
  - color select
  - pattern select
- In any select mode, turn the dial to cycle through options.
  Press the rotary encoder to select an option and move to the next state.
*/

#include <led_bars.h>
#include <Encoder.h>

#ifdef __AVR__
 #include <avr/power.h>
#endif

#define RE_CHAN1_PIN 2
#define RE_CHAN2_PIN 3
#define RE_BUTTON_PIN 4

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

Encoder knob(RE_CHAN1_PIN, RE_CHAN2_PIN);

enum op_states
{
  off,
  on,
  color_select,
  pattern_select,
};
op_states current_state = on;

long prev_knob_position  = 0;
int button_state = HIGH;
int prev_button_state = HIGH;

uint32_t last_debounce_time = 0;
uint32_t debounce_delay = 50;

uint32_t last_edit_time = millis();

void handle_clockwise() {
  switch (current_state) {
    case color_select:
      bars.next_color();
      break;
    case pattern_select:
      bars.next_pattern();
      break;
    default:
      ;
  }
}

void handle_counterclockwise() {
  switch (current_state) {
    case color_select:
      bars.prev_color();
      break;
    case pattern_select:
      bars.prev_pattern();
      break;
    default:
      ;
  }
}

// Handle state changes for editing modes, on or off
void next_mode() {
  switch (current_state) {
    case color_select:
      current_state = pattern_select;
      break;
    case pattern_select:
      current_state = off;
      break;
    case off:
      current_state = on;
      break;
    case on:
      current_state = color_select;
      break;
    default:
      ;
  }
}

void handle_knob_changes() {
  long knob_position = knob.read();
  // The rotary encoder tracks the number of button state changes
  // in a dial turn. This means the two channels will switch from off to on to off
  // making for 4 state changes in a single turn
  if (knob_position % 4 == 0) {
    if (knob_position > prev_knob_position) {
      last_edit_time = millis();
      handle_clockwise();
    }
    if (knob_position < prev_knob_position) {
      last_edit_time = millis();
      handle_counterclockwise();
    }
    prev_knob_position = knob_position;
  }
}

void handle_button_press() {
  int button_reading = digitalRead(RE_BUTTON_PIN);
  if (button_reading != prev_button_state) {
    last_debounce_time = millis();
  }

  // Software check for button debounce
  if ((millis() - last_debounce_time) > debounce_delay) {
    if (button_reading != button_state) {
      button_state = button_reading;
      if (button_state == HIGH) {
        next_mode();
      }
    }
  }
  prev_button_state = button_reading;
}

void setup() {
  pinMode(RE_BUTTON_PIN, INPUT);
  bars.begin();
} 

void loop() {
  // The Encoder library uses both external interrupts on a Pro/Mini so we
  // must poll for a direct button press first.
  handle_button_press();
  if (current_state == off) {
    bars.off();
  }
  else {
    if (current_state != on) {
      handle_knob_changes();

      // If nothing has changed for a bit then break out
      // of the edit state
      if (millis() - last_edit_time > 10000 ) {
        current_state = on;
      }
    }
    bars.render();
  }
}
