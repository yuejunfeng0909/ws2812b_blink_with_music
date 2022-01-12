#include <arduinoFFT.h>
#include <Adafruit_NeoPixel.h>

#define PIN          0  // digital pin used to drive the LED strip
#define NUMPIXELS       60  // number of LEDs on the strip
#define MAX_BRIGHTNESS  255

#define AUDIO_IN_PIN 26
#define SUBTRACT_PIN 27
#define MULTIPLIER_PIN 28

#define SAMPLES 32
#define SAMPLING_FREQ 20000

#define COLOR_SPEED 200
#define SMOOTHING_RATIO 64
#define DELAY_SMOOTHING 150

unsigned long delay_time;

unsigned int next_hue_updated_time;

unsigned int sampling_period_us;
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

// LED color
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500
uint32_t color;
unsigned long last_brightness;
unsigned int hueShift;
unsigned long brightness;
unsigned long time_smoothed_brightness;
unsigned long newColor;
float multiplier;
int subtract_val;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));

  pixels.begin();
  pixels.setBrightness(MAX_BRIGHTNESS);

  delay_time = millis();
  next_hue_updated_time = delay_time;
  last_brightness = 0;
  hueShift = 0;
  brightness = 0;
  time_smoothed_brightness = 0;

  next_hue_updated_time = millis();
}

void loop() {
//  int iteration_time_taken = millis();
  
  // sample audio pin
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN);
    vImag[i] = 0;
    while (micros() < (newTime + sampling_period_us)) {/* waiting */}
  }
  
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  subtract_val = analogRead(SUBTRACT_PIN)/5.0;
  multiplier = analogRead(MULTIPLIER_PIN)/500.0;
  
  brightness = max(0, min(255, ((float)vReal[(int)FFT.MajorPeak() * SAMPLES / SAMPLING_FREQ] - subtract_val) * multiplier));
  if (FFT.MajorPeak() < 20.0) {
    brightness = 0;
  }
  if (brightness > last_brightness) {
    time_smoothed_brightness = brightness;
    delay_time = millis() + DELAY_SMOOTHING;
  } else {
    if (millis() > delay_time) {
      time_smoothed_brightness = brightness + last_brightness * (SMOOTHING_RATIO - 1);
      time_smoothed_brightness /= SMOOTHING_RATIO;
    } else {
      time_smoothed_brightness = last_brightness;
    }
  }

  Serial.println(time_smoothed_brightness);
  
  last_brightness = time_smoothed_brightness;
   
  if (millis() > next_hue_updated_time) {
    hueShift ++;
    next_hue_updated_time = next_hue_updated_time + COLOR_SPEED;
  }
  
  for (int i = 0; i < NUMPIXELS / 2; i++) {
    color = pixels.ColorHSV(65535 * (i + hueShift) / NUMPIXELS, 255, time_smoothed_brightness);
    
    pixels.setPixelColor(i, color);
    pixels.setPixelColor(NUMPIXELS - 1 - i, color);
    
  }
  pixels.show();
  
}
