#include "Arduino.h"
#include "led_bars.h"
#include "math.h"
#include "Adafruit_NeoPixel.h"
#include <EEPROM.h>


// Math Helpers

int sine_wave(int amp, float freq, long time, int offset) {
  return amp * sin(2 * PI * freq * time) + offset;
}

int sawtooth_wave(int amp, float freq, long time, int offset) {
  return (-2 * amp / PI) * atan(1 / tan(PI * time * freq)) + offset;
}

int rev_sawtooth_wave(int amp, float freq, long time, int offset) {
  return (2 * amp) - ((-2 * amp / PI) * atan(1 / tan(PI * time * freq)) + offset);
}

int triangle_wave(int amp, float freq, long time, int offset) {
  return ((2 * amp / PI) * asin(sin(2 * PI * time * freq))) + offset;
}


// Control Helpers

/**
* This first line is a brief description.
*
* The rest of the lines are a more detailed description of the
* function that outlines what it does and anything interesting about
* how it does it.
*
* @param value Pointer ref for the value being changed
* @param max Maximum value allowed, determines when to clamp or rollover
* @param step Amount 
*/

// Increase a variable by a specified step with capability to hold at a maximum value
// or rollover to a different value.
void inc_value(uint8_t* value, int max, int step = 1, bool clamp = false, int wrap = 0) {
  if (*value >= max) {
    if (clamp) {
      *value = max;
    }
    else {
      *value = wrap;
    }
  }
  else {
    *value += step;
  }
}

// Decrease a variable by a specified step with capability to hold at a maximum value
// or rollover to a different value.
void dec_value(uint8_t* value, int min, int step = 1, bool clamp = false, int wrap = 0) {
  if (*value <= min) {
    if (clamp) {
      *value = min;
    }
    else {
      *value = wrap;
    }
  }
  else {
    *value -= step;
  }
}

// Is a position already occupied in a particle array?
bool is_in(int position, particle arr[]) {
  for (int i = 0; i < 10; i++) {
    if (position == arr[i].position) {
      return true;
    }
  }
  return false;
}

float float_rand( float min, float max) {
  float scale = rand() / (float)RAND_MAX;
  return min + scale * (max - min);
}

// LED_Bars control functions

void LED_Bars::next_color() {
  inc_value(&color_index, num_colors - 1);
}

void LED_Bars::prev_color() {
  dec_value(&color_index, 0, 1, false, num_colors - 1);
}

void LED_Bars::next_pattern() {
  inc_value(&pattern_index, num_patterns - 1);
}

void LED_Bars::prev_pattern() {
  dec_value(&pattern_index, 0, 1, false, num_patterns - 1);
}

void LED_Bars::inc_color_hue() {
  inc_value(&color_hue, 255, 1);
}

void LED_Bars::dec_color_hue() {
  dec_value(&color_hue, 0, 1, false, 255);
}

void LED_Bars::inc_brightness() {
  inc_value(&brightness, 255, 5, true);
}

void LED_Bars::dec_brightness() {
  dec_value(&brightness, 0, 5, true);
}

void LED_Bars::load_values() {
  EEPROM.get(color_index_addr, color_index);
  EEPROM.get(pattern_index_addr, pattern_index);
  EEPROM.get(brightness_addr, brightness);
  EEPROM.get(color_hue_addr, color_hue);
}

void LED_Bars::save_values() {
  EEPROM.put(color_index_addr, color_index);
  EEPROM.put(pattern_index_addr, pattern_index);
  EEPROM.put(brightness_addr, brightness);
  EEPROM.put(color_hue_addr, color_hue);
}

void LED_Bars::rand() {
  pattern_index = random(0, num_patterns);
  color_index = random(0, num_colors);
}

void LED_Bars::set_led_color(uint8_t x, uint8_t y, uint32_t color_value, uint8_t bright) {
  strip.setPixelColor(map_to_position(x, y), color_value, bright);
}

void LED_Bars::set_pattern(pattern_func func) {
  unsigned int index = 0;
  for (int i = 0; i < num_patterns; i++) {
    if (func == patterns[i]) {
      index = i;
      break;
    }
  }
  pattern_index = index;
}

void LED_Bars::set_color(color_func func) {
  unsigned int index = 0;
  for (int i = 0; i < num_colors; i++) {
    if (func == colors[i]) {
      index = i;
      break;
    }
  }
  color_index = index;
}

void LED_Bars::begin() {
  strip.begin();
  off();
}

void LED_Bars::off() {
  if (is_off == false) {
    strip.clear();
    strip.show();
    is_off = true;
  }
}

void LED_Bars::render() {
  is_off = false;
  strip.clear();
  pattern();
  strip.show();
}

unsigned int LED_Bars::map_to_position(uint8_t x, uint8_t y) {
  segment seg;
  unsigned int pos;
  if (vertical == true) {
    seg = segments[x];
    pos = y;
  } else {
    seg = segments[y];
    pos = x;
  }
    
  if (seg.reverse == true) {
    return seg.first_position - pos;
  } else {
    return seg.first_position + pos;
  }
}


// Pattern functions

// General accessor function to get the currently selected pattern
void LED_Bars::pattern() {
  pattern_func pattern_f = patterns[pattern_index];
  return (this->*pattern_f)();
}

// Fill all the leds
void LED_Bars::fill() {
  for (int i = 0; i < n_segments; i++) {
    for (int j = 0; j < led_per_segment; j++) {
      set_led_color(i, j, color(j, i, 0.0), 125);
    }
  }
}

// Fill all leds but glow between off and on every 5 seconds
void LED_Bars::glow() {
  int bright = sine_wave(125, 0.0002, millis(), 125);
  for (int i = 0; i < n_segments; i++) {
    for (int j = 0; j < led_per_segment; j++) {
      set_led_color(i, j, color(j, i, 0.0), bright);
    }
  }
}

/*
Helper function for rendering constant and repetative motion.

Used to remove some repetative code with the bouncer and chaser functions.
This should be used with some sort of waveform function to show constant/oscilating motion.

@param n_lines The number of lines to show
@param freq The frequency of the repeating movements
@param drift Offset the led placement for a wavy effect
@param *pos_func Function pointer for waveform calculation of position displacement
*/
void LED_Bars::calc_bounce(
  int n_lines, float freq, bool drift,
  int (*pos_func)(int amp, float freq, long time, int offset)
  ) {
  int amplitude = led_per_segment / 2;
  int line_offset = ( 1 / freq ) / n_lines;
  int pos_offset = drift == true ? 10 : 0;

  for (int i = 0; i < n_lines; i++) {
    for (int j = 0; j < n_segments; j++) {
      int time_offset = (j * pos_offset) + (i * line_offset);
      int pos = pos_func(amplitude, freq, millis() + time_offset, amplitude);
      set_led_color(j, pos, color(pos, j, 0.0), 125);
    }
  }
}

// Show three lines evenly bouncing between top and bottom 
void LED_Bars::bouncer() {
  calc_bounce(3, 0.002, false, sine_wave);
}

// Show three lines bouncing between top and bottom where
// each line is a bit wavy
void LED_Bars::bouncer_wave() {
  calc_bounce(3, 0.002, true, sine_wave);
}

// Show three lines evenly moving from top to bottom 
void LED_Bars::chaser() {
  calc_bounce(3, 0.002, false, sawtooth_wave);
}

// Show three lines evenly moving from top to bottom where
// each line will be a bit wavy
void LED_Bars::chaser_wave() {
  calc_bounce(3, 0.002, true, sawtooth_wave);
}

// Show three lines evenly moving from bottom to top
void LED_Bars::reverse_chaser() {
  calc_bounce(3, 0.002, false, rev_sawtooth_wave);
}

// Show three lines evenly moving from bottom to top where
// each line will be a bit wavy
void LED_Bars::reverse_chaser_wave() {
  calc_bounce(3, 0.002, true, rev_sawtooth_wave);
}

/*
Handle scattered and static blinking lights (sparkles) as particles.

Used to remove some repetative code with the sparkle functions. Intended for creating
patterns with non moving blinking lights.

@param drift Show led colors with a slightly varying hue
*/
void LED_Bars::cycle_sparkles(bool drift) {
  unsigned int bright;
  unsigned int pos;
  float freq;
  int hue_drift_value;

  for (int i = 0; i < n_segments; i++) {
    for (int j = 0; j < particle_count; j++) {
      // Create a new sparkle instance with a unique position
      // and frequency, a zero freq indicates this sparkle has
      // finished it's animation cycle and can be replaced
      if (particles[i][j].freq == 0.0) {
        do {
          pos = random(0, led_per_segment);
        } while (is_in(pos, particles[i]));
        // To get the desired "glow" effect a frequency must be chosen
        // that is currently at a minimum in its sinusoid cycle
        // This causes the particle to go from 0->255->0 in brightness smoothly
        do {
          freq = float_rand(0.001, 0.0001);
          bright = sine_wave(125, freq, millis(), 125);
        } while (bright != 0);

        particles[i][j].position = pos;
        particles[i][j].freq = freq;
        particles[i][j].hue_drift = random(-1500, 1501);
        particles[i][j].start_time = millis();
      } else {
        // If the sparkle has already ran for a cycle then it is removed
        bright = sine_wave(125, particles[i][j].freq, millis(), 125);
        if (bright == 0 && (millis() - particles[i][j].start_time) > 100) {
          particles[i][j].freq = 0.0;
        } else {
          // Render valid sparkle particles
          hue_drift_value = drift == true ? particles[i][j].hue_drift : 0;
          pos = particles[i][j].position;
          set_led_color(i, pos, color(pos, i, hue_drift_value), bright);
        }
      }
    }
  }
}

// Create a series of random and slow blinking lights
void LED_Bars::sparkles() {
  cycle_sparkles(false);
}

// Create a series of random and slow blinking lights
void LED_Bars::sparkles_drift() {
  cycle_sparkles(true);
}


// Motion based patterns

// Simple linear displacement calculation from top to bottom
int moving_calc(unsigned long time, int count, float vel) {
  uint32_t anim_speed = 500;
  float init_v = count / anim_speed;
  return (init_v * time);
}

// Simple linear displacement calculation from bottom to top
int upward_calc(unsigned long time, int count, float vel) {
  int value = moving_calc(time, count, vel);
  return (count - 1) - value;
}

/*
To create a falling effect the light positions are calculated by a simple kinematics displacement equation:
  dist = init_v * time + 0.5 * acc * time^2
The values below work as configurations for this. By default the positions move with velocity and acceleration
at a rate that they will reach full velocity (led_per_segment/anim_speed) by the bottom. Units in led/ms.
*/
int falling_calc(unsigned long time, int count, float vel) {
  uint32_t anim_speed = 1000;
  float init_v = 0.01 * (count / anim_speed);
  float acc = (2 * (count - (init_v * anim_speed))) / pow(anim_speed, 2);
  return (init_v * time) + (0.5 * acc * pow(time, 2));
}

// Same as above but moves from bottom to top
int rising_calc(unsigned long time, int count, float vel) {
  int value = falling_calc(time, count, vel);
  return (count - 1) - value;
}

/*
Creates a falling effect with variable speeds using the same equations above.
This however expects a velocity to be provided and is intended for use with
the `particle` struct.
*/
int falling_calc_rand(unsigned long time, int count, float vel) {
  uint32_t anim_speed = 1000;
  float init_v = 0.01 * (count / anim_speed);
  float acc = (2 * (count - (init_v * anim_speed))) / pow(anim_speed, 2);
  return (vel * time) + (0.5 * acc * pow(time, 2));
}

// Same as above but moves from bottom to top
int rising_calc_rand(unsigned long time, int count, float vel) {
  int value = falling_calc_rand(time, count, vel);
  return (count - 1) - value;
}

/*
Handles updates for any particles that are part of a pattern, removing, updating or creating as needed.

Used to remove some repetative code with the various particle based patterns. This iterates over any particles
and calculates their displacement and determines if they are out of bounds and should be removed. If `no_gen` is false
then a new particle will be created and shown on the `active_seg`. This allows configuration of various falling or moving
effects.

@param active_seg Which segment a new particle should appear on
@param no_gen Should a new particle be generated
@param glow Apply a variable brightness for a sparkle effect
@param hue_drift Apply a variable hue to the led color
@param pos_func Function for calculating particle displacement over time
*/
void LED_Bars::cycle_particles(
  unsigned int active_seg, bool no_gen, bool glow, bool hue_drift,
  int (*pos_func)(unsigned long time, int count, float vel)
  ) {
  unsigned long time;
  unsigned long particle_time;
  int position;
  int bright;
  float vel;
  float freq;
  int hue_drift_value;

  for (int i = 0; i < n_segments; i++) {
    for (int j = 0; j < particle_count; j++) {
      particle_time = particles[i][j].start_time;

      // Generate a single new position once per segment
      if (active_seg == i && particle_time == 0 && no_gen == false) {
        particles[i][j].start_time = millis();
        particles[i][j].vel = float_rand(0.0001, 0.01);
        particles[i][j].freq = float_rand(0.0001, 0.001);
        particles[i][j].hue_drift = random(-1500, 1501);
        particle_time = particles[i][j].start_time;
        no_gen = true;
      }

      // Calculate position offset from the top
      time = millis() - particle_time;
      vel = particles[i][j].vel;
      freq = particles[i][j].freq;
      position = pos_func(time, led_per_segment, vel);

      // Show any active position within the led boundary and
      // release positions that fall out of bounds
      if (position >= led_per_segment || position < 0) {
        particles[i][j].start_time = 0;
      } else {
        // Only render a zero position if it is being generated in this cycle,
        // without this the other zero position are always shwon at the top
        if (!(position == 0 && i != active_seg)) {
          bright = glow == true ? sine_wave(125, freq, millis(), 125) : 125;
          hue_drift_value = hue_drift == true ? particles[i][j].hue_drift : 0;
          set_led_color(i, position, color(position, i, hue_drift_value), bright);
        }
      }
    }
  }
}

/*
Oscilate between led segments in a triangle pattern every quater of a second

Helper fun to remove some repeated code. To create a wave effect the light positions are generated at the top in an oscilating pattern.
Calculate the active segment that can generate a position here. For this use case
the triangle wave looks the best as it is completley linear between its amplitude and leaves few gaps.

@param n_segments Should be the same as the number of led segments

@return The available segment index
*/
int gen_seg(int n_segments) {
  int amplitude = n_segments / 2;
  float frequency = 0.004;
  return triangle_wave(amplitude, frequency, millis(), amplitude);
}

// Show a constantly moving waveform
void LED_Bars::waves() {
  int active_seg = gen_seg(n_segments);

  bool no_gen = true;
  if (prev_seg != active_seg) {
    prev_seg = active_seg;
    no_gen = false;
  }

  cycle_particles(active_seg, no_gen, false, false, moving_calc);
}

// Shows a vertical wave like pattern that falls faster as it moves
void LED_Bars::falling_waves() {
  int active_seg = gen_seg(n_segments);

  bool no_gen = true;
  if (prev_seg != active_seg) {
    prev_seg = active_seg;
    no_gen = false;
  }

  cycle_particles(active_seg, no_gen, false, false, falling_calc);
}

// Show random falling lights of varying speeds
void LED_Bars::falling_rain() {
  int gen_seg = random(0, n_segments);

  bool no_gen = true;
  if ((millis() - last_time) > random(50, 150)) {
    last_time = millis();
    no_gen = false;
  }

  cycle_particles(gen_seg, no_gen, false, false, falling_calc_rand);
}

// Show random falling lights of varying speeds that slowly blink
void LED_Bars::falling_sparkles() {
  int gen_seg = random(0, n_segments);
  
  bool no_gen = true;
  if ((millis() - last_time) > random(50, 150)) {
    last_time = millis();
    no_gen = false;
  }

  cycle_particles(gen_seg, no_gen, true, false, falling_calc_rand);
}

// Show random falling lights of varying speeds that slowly blink
// and apply a color variation for a sort of shimmer
void LED_Bars::falling_drift_sparkles() {
  int gen_seg = random(0, n_segments);

  bool no_gen = true;
  if ((millis() - last_time) > random(50, 150)) {
    last_time = millis();
    no_gen = false;
  }

  cycle_particles(gen_seg, no_gen, true, true, falling_calc_rand);
}

// Shows a vertical wave like pattern that falls faster as it moves
// with lights that slowly blink and apply a color variation for a sort of shimmer
void LED_Bars::falling_drift_sparkle_waves() {
  int active_seg = gen_seg(n_segments);

  bool no_gen = true;
  if (prev_seg != active_seg) {
    prev_seg = active_seg;
    no_gen = false;
  }

  cycle_particles(active_seg, no_gen, true, true, falling_calc);
}

// The same pattern as above but this on goes from bottom to top
void LED_Bars::rising_drift_sparkle_waves() {
  int active_seg = gen_seg(n_segments);

  bool no_gen = true;
  if (prev_seg != active_seg) {
    prev_seg = active_seg;
    no_gen = false;
  }

  cycle_particles(active_seg, no_gen, true, true, rising_calc);
}

// Push a point onto a point array and drop the last point value
void arr_push(point value, point arr[], unsigned int arr_size) {
  point buffer1 = value;
  point buffer2;
  for (int i = 0; i < arr_size; i++) {
    buffer2 = arr[i];
    arr[i] = buffer1;
    buffer1 = buffer2;
  }
}

bool point_eq(point point1, point point2) {
  return point1.x == point2.x && point1.y == point2.y;
}

// Is a given point already in a point array
bool point_in_arr(point pnt, point arr[], unsigned int arr_size) {
  for (int i = 0; i < arr_size; i++) {
    if (point_eq(pnt, arr[i]) == true) {
      return true;
    }
  }
  return false;
}

// Shuffle an array
void shuffle(int *array, size_t n) {
  size_t i;
  for (i = 0; i < n - 1; i++) {
    size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
    int t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

point random_adjacent(point orig, int val) {
  point pnt;
  switch (val)
  {
  case 0:
    pnt = { .x = orig.x + 1, .y = orig.y };
    break;
  case 1:
    pnt = { .x = orig.x - 1, .y = orig.y };
    break;
  case 2:
    pnt = { .x = orig.x, .y = orig.y + 1 };
    break;
  case 3:
    pnt = { .x = orig.x, .y = orig.y - 1 };
    break;
  default:
    pnt = { .x = orig.x, .y = orig.y - 1 };
    break;
  }
  return pnt;
}

/*
Return the positions above, below, left and right of a given point.
The order of the returned points are somewhat random to remove
affinity for the directionality of the snakes' movements
*/
point* Snakes::adjacent_points(point pnt) {
  int disp[4] = { 0, 1, 2, 3 };
  shuffle(disp, 4);
  static point next_points[4];
  for (int i = 0; i < 4; i++) {
    next_points[i] = random_adjacent(pnt, disp[i]);
  }
  return next_points;
}

// Does a given point intersect with any other points
bool Snakes::point_collision(point pnt) {
  for (int i = 0; i < snake_count; i++) {
    if (point_in_arr(pnt, snake_insts[i]->points, snake_insts[i]->length) == true) {
      return true;
    }
  }
  return false;
}

// Is a given point within the led space and not occupied
bool Snakes::valid_point(point pnt) {
  uint8_t x = pnt.x;
  uint8_t y = pnt.y;

  if (x >= width) {
    return false;
  } else if (y >= height) {
    return false;
  }
  if (point_collision(pnt) == true) {
    return false;
  }
  return true;
}

snake* Snakes::create_snake() {
  uint8_t length = random(5, 10);
  point pnt;
  snake *snake_inst = malloc(sizeof(snake) + length * sizeof(point *));

  do {
    pnt = { .x = random(0, width), .y = random(0, height) };
  } while (!(valid_point(pnt) == true));
  for (int i = 0; i < length; i++) {
    snake_inst->points[i] = pnt;  
  }
  snake_inst->length = length;
  snake_inst->hue_drift = random(-3000, 3001);
  snake_inst->start_time = millis();
  snake_inst->delay = random(250, 750);
  return snake_inst;
}

void Snakes::remove_snake(uint8_t index) {
  free(snake_insts[index]);
}

void Snakes::move_snake(uint8_t index) {
  point pnt;
  pnt = snake_insts[index]->points[0];

  point* next_pnts = adjacent_points(pnt);
  int k = 0;
  for (k = 0; k < 4; k++) {
    if (valid_point(*(next_pnts + k)) == true) {
      arr_push(*(next_pnts + k), snake_insts[index]->points, snake_insts[index]->length);
      break;
    }
  }
  if (k >= 4) {
    // Snake failed to find a valid spot and may be stuck
    remove_snake(index);
    snake_insts[index] = create_snake();
  }
}

// Show a series of moving segments similar to the classic snake game
void LED_Bars::moving_snakes() {
  point pnt;
  point pnt1;
  bool done = false;
  unsigned int bright = 125;
  snake* snake_inst;

  for (int i = 0; i < snakes.snake_count; i++) {
    snake_inst = snakes.snake_insts[i];
    for (int j = 0; j < snake_inst->length; j++) {
      pnt = snake_inst->points[j];
      if (j == 0) {
        bright = (millis() - snake_inst->start_time) * 125 / snake_inst->delay;
      } else if (j == snake_inst->length - 1 && !point_eq(pnt, snake_inst->points[j - 1])) {
        bright = (millis() - snake_inst->start_time) * 125 / snake_inst->delay;
        bright = 130 - bright;
      } else {
        bright = 125;
      }
      set_led_color(pnt.x, pnt.y, color(pnt.y, pnt.x, snake_inst->hue_drift), bright);
    }

    if (millis() - snake_inst->start_time > snake_inst->delay) {
      snake_inst->start_time = millis();
      snakes.move_snake(i);
    }
  }
}

void GameOfLife::random_board() {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      area[x][y] = (bool)random(0, 2);
    }
  }
}

bool GameOfLife::out_of_bounds(uint8_t x, uint8_t y) {
  return x >= width || x < 0 || y >= height || y < 0;
}

void GameOfLife::copy_area() {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      area[x][y] = area_n[x][y];
    }
  }
}

uint8_t GameOfLife::live_neighbors(uint8_t x, uint8_t y) {
  uint8_t count = 0;
  for (int i = -1; i < 2; i++) {
    for (int j = -1; j < 2; j++) {
      if ((i == 0 && j == 0) || out_of_bounds(x + i, y + j)) {
        continue;
      } else if (area[x + i][y + j] == true) {
        count++;
      }
    }
  }
  return count;
}

bool GameOfLife::alive(uint8_t x, uint8_t y) {
  uint8_t live_n = live_neighbors(x, y);
  bool status = area[x][y];
  // Any live cell with two or three live neighbours survives
  if (status == true && (live_n == 2 || live_n == 3)) {
    return true;
  }
  // Any dead cell with three live neighbours becomes a live cell
  else if (status == false && live_n == 3) {
    return true;
  }
  // All other live cells die in the next generation and all other dead cells
  // stay dead
  else {
    return false;
  }
}

void GameOfLife::generation() {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if (alive(x, y) == true) {
        area_n[x][y] = true;
      } else {
        area_n[x][y] = false;
      }
    }
  }
  copy_area();
}

void LED_Bars::life() {
  uint16_t alive_count = 0;
  bool generate = millis() - last_time > 100;
  for (int x = 0; x < game_of_life.width; x++) {
    for (int y = 0; y < game_of_life.height; y++) {
      if (game_of_life.area[x][y] == true) {
        alive_count++;
        set_led_color(x, y, color(y, x, 0), 125);
      }
    }
  }
  if (generate) {
    last_time = millis();
    game_of_life.generation();
  }
  if (alive_count <= 20) {
    game_of_life.random_board();
  }
}

// Color Functions

/*
Mapping for various colors and their hue values. Useful for gradient functions.

In working with the NeoPixel library I noticed the built in ColorHSV function wasn't always correct.
There's a good [explanation](https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp#L3213)
for some of the quirks but it didn't fully explain some issues. I figured my leds maybe don't have the precision/capability
for some color depth or my eyes are broken, but I needed something to reliably use hues for colors. So I made this.

My methodology was pretty ridiculous. First I needed to observe the various distinct colors and their hues.
The ColorHSV function is expecting a `uint16_t` so my hue values are split within it's boundaries (0-65535).
I split the led strips into 12 sections, one for each primary, secondary and tertiary color and displayed them in order.
Most looked good but the red to yellow transitions showed little variation and that colors around blue were too blue.
A bit of tuning and observation led to these numbers for an accurate color mapping for my leds.
*/
typedef struct Hues {
  uint16_t max_hue = 65535;
  uint32_t red = (0 * 65535 / 12);
  uint32_t vermillion = (1.5 * 65535 / 24);
  uint32_t orange = (2 * 65535 / 24);
  uint32_t amber = (3 * 65535 / 24);
  uint32_t yellow = (2 * 65535 / 12);
  uint32_t lime = (3 * 65535 / 12);
  uint32_t green = (4 * 65535 / 12);
  uint32_t teal = (5 * 65535 / 12);
  uint32_t cyan = (6 * 65535 / 12);
  uint32_t blue = (8 * 65535 / 12);
  uint32_t violet = (9 * 65535 / 12);
  uint32_t purple = (9.5 * 65535 / 12);
  uint32_t pink = (10.5 * 65535 / 12);
  uint32_t magenta = (11 * 65535 / 12);
  uint32_t vibrant_red = (11.5 * 65535 / 12);
} hues;
hues color_hues;

uint32_t LED_Bars::from_hue(uint16_t hue, int drift) {
  return strip.gamma32(strip.ColorHSV(hue + drift)); 
}

/*
Map a vertical postion to a color value that creates a color gradient.

Helper function to generalize some functionality. This function is used for creating color gradients
across a vertical axis. Multiple colors can be set to create a evenly placed gradient for each color.

@param pos vertical position to map
@param colorset[] array of color hues that define gradient points
@param n_colors number of color values in the array

@return gamma corrected, 32 bit packed color value
*/
uint32_t LED_Bars::vertical_gradient(int pos, uint16_t color_set[], int n_colors) {
  uint16_t partition = map(pos, 0, led_per_segment, 0, n_colors - 1);
  uint16_t next_partition = partition + 1;

  uint16_t min_pos = led_per_segment * ((float)partition / (float)(n_colors - 1));
  uint16_t max_pos = led_per_segment * ((float)next_partition / (float)(n_colors - 1));
  uint16_t hue = map(
    pos, min_pos, max_pos,
    color_set[partition], color_set[next_partition]
  );

  return strip.gamma32(strip.ColorHSV(hue)); 
}

/*
Map a vertical postion to a section containing a single color

Helper function to generalize some functionality. This function is used for creating individual color sections
across a vertical segment. Multiple colors can be set to create a multiple sections per color.

@param pos vertical position to map
@param *colorset array pointer of color hues that define the color sections
@param n_colors number of color values in the array

@return gamma corrected, 32 bit packed color value
*/
uint32_t LED_Bars::vertical_partitions(int pos, uint16_t *color_set, uint16_t n_colors) {
  uint32_t partition = map(pos, 0, led_per_segment, 0, n_colors);
  return strip.gamma32(strip.ColorHSV(*(color_set + partition)));
}

uint32_t LED_Bars::color(int pos, int seg, int drift) {
  color_func color_f = colors[color_index];
  return (this->*color_f)(pos, seg, drift);
}

uint32_t LED_Bars::red(int pos, int seg, int drift) {
  return from_hue(color_hues.red, drift);
}

uint32_t LED_Bars::vermillion(int pos, int seg, int drift) {
  return from_hue(color_hues.vermillion, drift);
}

uint32_t LED_Bars::orange(int pos, int seg, int drift) {
  return from_hue(color_hues.orange, drift);
}

uint32_t LED_Bars::amber(int pos, int seg, int drift) {
  return from_hue(color_hues.amber, drift);
}

uint32_t LED_Bars::yellow(int pos, int seg, int drift) {
  return from_hue(color_hues.yellow, drift);
}

uint32_t LED_Bars::lime(int pos, int seg, int drift) {
  return from_hue(color_hues.lime, drift);
}

uint32_t LED_Bars::green(int pos, int seg, int drift) {
  return from_hue(color_hues.green, drift);
}

uint32_t LED_Bars::teal(int pos, int seg, int drift) {
  return from_hue(color_hues.teal, drift);
}

uint32_t LED_Bars::cyan(int pos, int seg, int drift) {
  return from_hue(color_hues.cyan, drift);
}

uint32_t LED_Bars::blue(int pos, int seg, int drift) {
  return from_hue(color_hues.blue, drift);
}

uint32_t LED_Bars::violet(int pos, int seg, int drift) {
  return from_hue(color_hues.violet, drift);
}

uint32_t LED_Bars::purple(int pos, int seg, int drift) {
  return from_hue(color_hues.purple, drift);
}

uint32_t LED_Bars::pink(int pos, int seg, int drift) {
  return from_hue(color_hues.pink, drift);
}

uint32_t LED_Bars::magenta(int pos, int seg, int drift) {
  return from_hue(color_hues.magenta, drift);
}

uint32_t LED_Bars::vibrant_red(int pos, int seg, int drift) {
  return from_hue(color_hues.vibrant_red, drift);
}

uint32_t LED_Bars::white(int pos, int seg, int drift) {
  return strip.Color(255, 255, 255);
}

uint32_t LED_Bars::red_to_yellow(int pos, int seg, int drift) {
  uint16_t colors[2] = { color_hues.red, color_hues.yellow };
  return vertical_gradient(pos, colors, 2);
}

uint32_t LED_Bars::teal_to_purple(int pos, int seg, int drift) {
  uint16_t colors[2] = { color_hues.teal, color_hues.pink };
  return vertical_gradient(pos, colors, 2);
}

uint32_t LED_Bars::blue_magenta_blue(int pos, int seg, int drift) {
  uint16_t colors[3] = { color_hues.blue, color_hues.magenta, color_hues.blue };
  return vertical_gradient(pos, colors, 3);
}

uint32_t LED_Bars::rainbow(int pos, int seg, int drift) {
  uint16_t colors[2] = { 0, color_hues.max_hue };
  return vertical_gradient(pos, colors, 2);
}

uint32_t LED_Bars::red_green_blue(int pos, int seg, int drift) {
  uint16_t colors[3] = {
    color_hues.red, color_hues.green, color_hues.blue
  };
  return vertical_partitions(pos, colors, 3);
}

uint32_t LED_Bars::all_colors(int pos, int seg, int drift) {
  uint16_t colors[15] = {
    color_hues.red, color_hues.vermillion, color_hues.orange,
    color_hues.amber, color_hues.yellow, color_hues.lime,
    color_hues.green, color_hues.teal, color_hues.cyan,
    color_hues.blue, color_hues.violet, color_hues.purple,
    color_hues.pink, color_hues.magenta, color_hues.vibrant_red,
  };
  return vertical_partitions(pos, colors, 15);
}

uint32_t LED_Bars::magenta_yellow_cyan(int pos, int seg, int drift) {
  uint16_t colors[3] = {
    color_hues.magenta, color_hues.yellow, color_hues.cyan
  };
  return vertical_partitions(pos, colors, 3);
}

uint32_t LED_Bars::teal_cyan_magenta(int pos, int seg, int drift) {
  uint16_t colors[3] = {
    color_hues.teal, color_hues.cyan, color_hues.magenta
  };
  return vertical_partitions(pos, colors, 3);
}

// TODO: Patterns appear faster when these are used and I'm not sure why
uint32_t LED_Bars::rainbow_shift(int pos, int seg, int drift) {
  int hue = sawtooth_wave((100 / 2), 0.00001, millis(), (100 / 2));
  hue = map(hue, 0, 100, 0, color_hues.max_hue);
  return strip.gamma32(strip.ColorHSV(hue));
}

uint32_t LED_Bars::green_cyan_shift(int pos, int seg, int drift) {
  int hue = triangle_wave((100 / 2), 0.000016, millis(), (100 / 2));
  hue = map(hue, 0, 100, color_hues.green, color_hues.cyan);
  return strip.gamma32(strip.ColorHSV(hue));
}
