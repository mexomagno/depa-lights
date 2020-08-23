#include <Arduino.h>

#define RED 0
#define GREEN 1
#define BLUE 2
#define MIN_BRIGHTNESS 0

const unsigned char PINS[] = {3, 5, 6};
unsigned char COLORS[] = {0, 0, 0};

void setColors(unsigned char red, unsigned char green, unsigned char blue) {
  COLORS[RED] = map(red, 0, 255, MIN_BRIGHTNESS, 255);
  COLORS[GREEN] = map(green, 0, 255, MIN_BRIGHTNESS, 255);
  COLORS[BLUE] = map(blue, 0, 255, MIN_BRIGHTNESS, 255);
}
void setColors(unsigned char colors[]){
  setColors(colors[0], colors[1], colors[2]);
}

class Animation {
public: 
  virtual void start();
  virtual void step(long);
};

class WhiteFade: public Animation {
public:
  WhiteFade(long d) {
    sweep_delay_us = d; 
  }
  void start() {
    setColors(0, 0, 0);
    value = 0;
    direction = true;
  }
  void step(long dt_us) {
    double delta_value = (double)dt_us/(double)sweep_delay_us * 255;
    value = value + delta_value * (direction ? 1 : -1);
    // constrain value
    value = value < 0 ? 0 : value > 255 ? 255 : value;
    // change direction
    direction = value == 0 ? true : value == 255 ? false : direction;
    setColors((int)value, (int)value, (int)value);
  }
private:
  long sweep_delay_us;
  double value;
  bool direction;
};

class HueRotation: public Animation {
public:
  HueRotation(long d) {
    sweep_delay_us = d;
  }
  void start() {
    setColors(0, 0, 255);
    value = 0;
    color = RED;
  }
  void step(long dt_us) {
    double delta_value = (double)dt_us/(double)sweep_delay_us * 255;
    value = value + delta_value;
    // shift color if needed
    color = value > 255 ? (color + 1) % 3 : color;
    // constrain value
    value = value > 255 ? 0 : value;
    unsigned char red = color == RED ? value : color == GREEN ? 255 - value : 0;
    unsigned char green = color == GREEN ? value : color == BLUE ? 255 - value : 0;
    unsigned char blue = color == BLUE ? value : color == RED ? 255 - value : 0;

    setColors((int)red, (int)green, (int)blue);
  }
private: 
  double value;
  unsigned char color;

  long sweep_delay_us; 
};

class Strobe: public Animation {
public:
  Strobe(long d) {
    delay = d;
  }
  void start() {
    setColors(0, 0, 0);
    on = false;
  }
  void step(long dt_us) {
    elapsed += dt_us;
    if (elapsed >= delay){
      on = !on;
      elapsed = elapsed - delay;
      setColors(on ? 255 : 0, on ? 255 : 0, on ? 255 : 0);
    }
  }
private: 
  bool on;
  long delay;
  long elapsed;
};

class ColorArray: public Animation {
public:
  ColorArray(long d, bool r) {
    delay_us = d;
    randomize = r;
  }
  void start() {
    index = 0;
    elapsed_us = 0;
    updateColor();
  }
  void step(long dt_us) {
    elapsed_us += dt_us;
    if (elapsed_us >= delay_us){
      elapsed_us = elapsed_us - delay_us;
      int arr_size = sizeof(colors)/sizeof(unsigned char[3]);
      unsigned char prev_index = index;
      index = randomize ? random(arr_size) : (index + 1) % arr_size;
      if (prev_index == index){
        index = (index + 1) % arr_size;
      }
      updateColor();
    }
  }
private: 
  long delay_us;
  long elapsed_us;
  bool randomize;
  unsigned char index; 
  const unsigned char colors[7][3] = {
    {255, 255, 255}, // white
    {255, 0, 0}, // red
    {0, 255, 0}, // green
    {0, 0, 255}, // blue
    {255, 255, 0}, // yellow
    {0, 255, 255}, // cyan
    {255, 0, 255} // purple
  };
  void updateColor(){
    setColors(colors[index]);
  }
};

Animation* animation;
Animation* animations[] = {
  new HueRotation(500000),
  new Strobe(30000),
  new WhiteFade(500000),
  new ColorArray(100000, true)
};
int anim_index = 0;
long shiftPeriod = 2000000;
long at0 = millis();
long adt = 0;

void shiftAnimation(){
  int arr_size = sizeof(animations)/sizeof(Animation*);
  if (arr_size <= 1)
    return;
  anim_index = (anim_index + 1) % arr_size;
  animation = animations[anim_index];
  animation->start();
}

void setup() {
  for (char i = 0; i < 3; i++){
    pinMode(PINS[i], OUTPUT);
  }
  animation = animations[0];
  animation->start();
}
const int FPS = 60;

long PERIOD_us = (long)(1.0/(double)(FPS) * 1000000);
long now;
long t0 = micros();
long dt = 0;

void update(long dt){
  animation->step(dt);
  analogWrite(PINS[RED], COLORS[RED]);
  analogWrite(PINS[GREEN], COLORS[GREEN]);
  analogWrite(PINS[BLUE], COLORS[BLUE]);
}

void loop() {
  now = micros();
  dt = now - t0;
  if (dt >= PERIOD_us) {
    update(dt);
    t0 = now + (dt - PERIOD_us);
  }

  adt = now - at0;
  if (adt >= shiftPeriod) {
    shiftAnimation();
    at0 = now + (adt - shiftPeriod);
  }
}