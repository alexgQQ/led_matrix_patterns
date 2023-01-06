/*
Library for creating and controlling moving patterns on multiple led strips.

The LED_Bars class contains an api for use in sketches. These contain functions for
generating patterns and colors of varying types. The class requires a definiton
of `segment` types. These define how the strips/segments are connected.
*/

#ifndef led_bars_h
#define led_bars_h

#include "Arduino.h"
#include "Adafruit_NeoPixel.h"

#ifndef LED_SEGMENTS
#define LED_SEGMENTS 1
#endif

// Default amount of particles for various animations
// TODO: Anything higher than 10 makes animations static, seems memory related
#ifndef LED_PARTICLES
#define LED_PARTICLES 10
#endif

/*
  A segment is a single strip of leds. Based on the current structure
  the segments are mounted vertically and connected from top to bottom.
  This makes the led indexing tricky as the initial input is at the top of 
  the first segment which then connects to the bottom of the second segment.
  The first led for the first segment is then at the top while the first led
  of the second segment is as the bottom. 
*/
typedef struct Segment {
  unsigned int first_position;
  bool reverse;
} segment;

typedef struct Particle {
  unsigned int position = 0;
  unsigned long start_time = 0;
  float vel = 0.0;
  float freq = 0.01;
  int hue_drift = 0;
} particle;

// Math helpers

int sine_wave(int amp, float freq, long time, int offset);
int sawtooth_wave(int amp, float freq, long time, int offset);
int triangle_wave(int amp, float freq, long time, int offset);
int rev_sawtooth_wave(int amp, float freq, long time, int offset);

void inc_value(uint8_t* value, int max, int step = 1, bool clamp = false, int wrap = 0);
void dec_value(uint8_t* value, int min, int step = 1, bool clamp = false, int wrap = 0);

bool is_in(int val, particle arr[]);
float float_rand( float min, float max);

int moving_calc(unsigned long time, int count, float vel);
int upward_calc(unsigned long time, int count, float vel);
int falling_calc(unsigned long time, int count, float vel);
int rising_calc(unsigned long time, int count, float vel);
int falling_calc_rand(unsigned long time, int count, float vel);
int rising_calc_rand(unsigned long time, int count, float vel);
int gen_seg(int n_segments);

class LED_Bars {

private:

  typedef uint32_t (LED_Bars::*color_func)(int, int, int);
  typedef void (LED_Bars::*pattern_func)();

  Adafruit_NeoPixel strip;
  bool is_off = true;
  bool vertical = true;

  // Member arrays have to be intiailized with a fixed static var
  segment segments[LED_SEGMENTS];

  uint8_t color_hue = 0;
  int color_hue_addr = 0;

  uint8_t brightness = 55;
  int brightness_addr = 3;

  unsigned int map_to_position(uint8_t x, uint8_t y);
  uint32_t vertical_gradient(int pos, uint16_t color_set[], int n_colors);
  uint32_t vertical_partitions(int pos, uint16_t *color_set, uint16_t n_colors);
  void cycle_particles(unsigned int active_seg, bool no_gen, bool glow, bool hue_drift, int (*pos_func)(unsigned long time, int count, float vel));
  uint32_t from_hue(uint16_t hue, int drift);
  void calc_bounce(int n_waves, float freq, bool drift, int (*pos_func)(int amp, float freq, long time, int offset));
  void cycle_sparkles(bool drift);

  particle particles[LED_SEGMENTS][LED_PARTICLES];
  int prev_seg = -1;
  unsigned long last_time = millis();
  const int particle_count = LED_PARTICLES;

  /*
  TODO: Using a dynamic array causes build errors and defining
  a const `num_patterns` and using that with the array construct
  also causes build errors, static/const class members also fail.
  To not duplicate this value I just compute the number of patterns
  based on this array size.
  */
  pattern_func patterns[17] = {
    &fill,
    &glow,
    &sparkles,
    &sparkles_drift,
    &chaser,
    &chaser_wave,
    &reverse_chaser,
    &reverse_chaser_wave,
    &bouncer,
    &bouncer_wave,
    &waves,
    &falling_waves,
    &falling_rain,
    &falling_sparkles,
    &falling_drift_sparkles,
    &falling_drift_sparkle_waves,
    &rising_drift_sparkle_waves,
  };
  int num_patterns = sizeof(patterns) / sizeof(patterns[0]);
  uint8_t pattern_index = 0;
  int pattern_index_addr = 1;
  void pattern();

  /*
  TODO: Using a dynamic array causes build errors and defining
  a const `num_colors` and using that with the array construct
  also causes build errors, static/const class members also fail.
  To not duplicate this value I just compute the number of colors based on the array size.
  */
  color_func colors[24] = {
    &red,
    &vermillion,
    &orange,
    &amber,
    &yellow,
    &lime,
    &green,
    &teal,
    &cyan,
    &blue,
    &violet,
    &purple,
    &pink,
    &magenta,
    &vibrant_red,
    &rainbow,
    &all_colors,
    &red_green_blue,
    &magenta_yellow_cyan,
    &red_to_yellow,
    &teal_to_purple,
    &teal_cyan_magenta,
    &blue_magenta_blue,
    &green_cyan_shift,
  };
  int num_colors = sizeof(colors) / sizeof(colors[0]);
  uint8_t color_index = 0;
  int color_index_addr = 2;
  uint32_t color(int pos, int seg, int drift);

public:

  uint16_t n_segments;
  uint16_t led_per_segment;

  LED_Bars(uint16_t n_segs, uint16_t led_per_seg, uint16_t data_pin, segment* segs) : strip(n_segs * led_per_seg, data_pin, NEO_GRB + NEO_KHZ800) {
    n_segments = n_segs;
    led_per_segment = led_per_seg;

    segment *old = segments;
    for(int i = 0; i < n_segs; ++i)
        *old++ = *segs++;
  };

  void begin();
  void off();
  void render();
  void save_values();
  void load_values();

  // Control functions
  void next_color();
  void prev_color();
  void inc_color_hue();
  void dec_color_hue();
  void next_pattern();
  void prev_pattern();
  void inc_brightness();
  void dec_brightness();
  void rand();
  void set_pattern(pattern_func func);
  void set_color(color_func func);

  // Pattern functions
  void fill();
  void glow();
  void waves();
  void falling_waves();
  void falling_rain();
  void bouncer();
  void bouncer_wave();
  void sparkles();
  void sparkles_drift();
  void chaser();
  void chaser_wave();
  void falling_sparkles();
  void falling_drift_sparkles();
  void falling_drift_sparkle_waves();
  void reverse_chaser();
  void reverse_chaser_wave();
  void rising_drift_sparkle_waves();

  // Color functions
  uint32_t red(int pos, int seg, int drift);
  uint32_t vermillion(int pos, int seg, int drift);
  uint32_t orange(int pos, int seg, int drift);
  uint32_t amber(int pos, int seg, int drift);
  uint32_t yellow(int pos, int seg, int drift);
  uint32_t lime(int pos, int seg, int drift);
  uint32_t green(int pos, int seg, int drift);
  uint32_t teal(int pos, int seg, int drift);
  uint32_t cyan(int pos, int seg, int drift);
  uint32_t blue(int pos, int seg, int drift);
  uint32_t violet(int pos, int seg, int drift);
  uint32_t purple(int pos, int seg, int drift);
  uint32_t pink(int pos, int seg, int drift);
  uint32_t vibrant_red(int pos, int seg, int drift);
  uint32_t magenta(int pos, int seg, int drift);
  uint32_t white(int pos, int seg, int drift);
  uint32_t rainbow(int pos, int seg, int drift);
  uint32_t all_colors(int pos, int seg, int drift);
  uint32_t red_to_yellow(int pos, int seg, int drift);
  uint32_t teal_to_purple(int pos, int seg, int drift);
  uint32_t rainbow_shift(int pos, int seg, int drift);
  uint32_t red_green_blue(int pos, int seg, int drift);
  uint32_t magenta_yellow_cyan(int pos, int seg, int drift);
  uint32_t teal_cyan_magenta(int pos, int seg, int drift);
  uint32_t green_cyan_shift(int pos, int seg, int drift);
  uint32_t blue_magenta_blue(int pos, int seg, int drift);

};

#endif