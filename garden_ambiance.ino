#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define DATA_PIN 6
#define BUTTON_PIN 3
#define LEDS_PER_STRIP 15
#define STRIP_COUNT 3
#define PIXELS (LEDS_PER_STRIP * STRIP_COUNT) 
#define MULTIPLIER 65536
#define PITCH 256
#define SPREAD 7
#define FADE_IN SPREAD
#define QUIET 4
#define DEBOUNCE 3
#define FADE_COUNT 256

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, DATA_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  pinMode(BUTTON_PIN, INPUT);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

bool buttonPressed(void)
{
  static uint8_t count = 0;
  if( ! digitalRead(BUTTON_PIN) ) {
    if(count < DEBOUNCE) {
      count++;
      if(count == DEBOUNCE) {
        return true;
      }
    }
  } else {
    count = 0;
  }
  return false;
}

void loop() {
  uint8_t wait = 1;
  while(1) {
    trackSun(strip.Color(255, 255, 32), wait);
    fadeOut(wait);
    fadeToColor(strip.Color(0, 0, 0), wait);
    fadeToColor(strip.Color(255, 255, 32), wait);
    fadeToColor(strip.Color(255, 0, 0), wait);
    fadeToColor(strip.Color(0, 255, 0), wait);
    fadeToColor(strip.Color(0, 0, 255), wait);
    fadeOut(wait);
  }
}

void fadeOut(uint32_t wait)
{
  while(1) {
    bool allZero = true;
    for(uint32_t i=0; i<strip.numPixels(); i++) {
      uint32_t color = scaleColor(strip.getPixelColor(i),99*MULTIPLIER/100);
      if(color) {
        allZero = false;
      }
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(wait);
    if(allZero) {
      return;
    }
  }
}

void trackSun(uint32_t color, uint8_t wait) {
  uint32_t start = 0;
  uint32_t end = (strip.numPixels() * PITCH) - 1;
  uint32_t fade_dist = FADE_IN * PITCH;
  uint32_t spread = SPREAD * PITCH;

  while(1) {
    for(uint32_t i=start; i<=end; i++) {
      uint32_t fade = MULTIPLIER;
      uint32_t edge_dist = min(distance(start, i), distance(end, i));
      if(edge_dist < fade_dist) {
        fade = MULTIPLIER * edge_dist / fade_dist;
      }
      positionSun(i, scaleColor(color, fade), spread, wait);
      if(buttonPressed()) {
        return;
      }
    }
    for(uint32_t i=start; i<=end/QUIET; i++) {
      positionSun(i, 0, spread, wait);
      if(buttonPressed()) {
        return;
      }
    }
  }
}

void fadeToColor(uint32_t color, uint8_t wait) {
  uint32_t prevColor = strip.getPixelColor(0);
  for(uint32_t fade = 0; fade < MULTIPLIER; fade += MULTIPLIER/FADE_COUNT) {
    setColor(scaleColor(color, fade) + scaleColor(prevColor, MULTIPLIER - fade), wait);
    if(buttonPressed()) {
      return;
    }
  }
  while(1) {
    setColor(color, wait);
    if(buttonPressed()) {
      return;
    }
  }
}

uint32_t distance(uint32_t ref, uint32_t pos) {
  if(pos > ref) {
    return pos - ref;
  } else {
    return ref - pos;
  }
}

uint32_t scaleColor(uint32_t color, uint32_t scale)
{
  uint32_t r = scale * ((color >> 16) & 0xff) / MULTIPLIER;
  uint32_t g = scale * ((color >> 8) & 0xff) / MULTIPLIER;
  uint32_t b = scale * ((color >> 0) & 0xff) / MULTIPLIER;
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

uint32_t attenuate(uint32_t color, uint32_t distance, uint32_t spread) {
  if(distance < spread) {
    uint32_t scale = MULTIPLIER * (spread - distance) / spread;
    return scaleColor(color, scale);
  } else {
    return 0;
  }
}

void positionSun(uint32_t pos, uint32_t color, uint32_t spread, uint8_t wait) {
  for(uint32_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, attenuate(color, distance(pos, i*PITCH), spread));
  }
  strip.show();
  delay(wait);
}

void setColor(uint32_t color, uint8_t wait) {
  for(uint32_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
  delay(wait);
}

