#include <Arduino.h>
#include <FastLED.h>
#include "arduinoFFT.h"

#define LED_PIN     13
#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define FRAMERATE   50

#define SENSITIVITY_PIN 33

#define LED_COLUMS  7
#define LED_ROWS    40
//#define LED_COLUMS  11
//#define LED_ROWS    15
#define NUM_LEDS    LED_COLUMS * LED_ROWS

// A tűz effekt színátmenete
DEFINE_GRADIENT_PALETTE( fire_gp ) {
0,   255,   0,   0,    //red
128, 255,  96,   0,    //orange
192, 255, 255,   0,    //bright yellow
255, 255, 255, 255 }; //full white
CRGBPalette16 firePalette = fire_gp;

// A méreg effekt színátmenete
DEFINE_GRADIENT_PALETTE( toxic_gp ) {
0,   0,   255,  58,    
90,  0,   164,  84,   
165, 255,   0, 199,    
255, 255,   0,   0 }; 
CRGBPalette16 toxicPalette = toxic_gp;

// A gomb lába
#define BUTTON_PIN  14

// A hangbemenet pinje
#define AUDIO_PIN   12
// A hang minimuma analog readen
#define AUDIO_MIN   2950
// A hang maximuma analog readen
#define AUDIO_MAX   4200

// Ha kell a serial port ne legyen kommentelve
//#define SERIAL_

// Ha nem kell a smoothing kommenteld ki
#define SMOOTHING
// A miximum érték amivel kissebb lehet a sáv előző értékéhez képest. [0.00-1.00]
#define SMOOTHNESS_CLAMP 0.05f

// A maximumot jelző kis fényecske kell-e, ha nem kommenteld ki
#define MAX_MARKER
// Az érték amivel esik a jelző képkockántként
#define MAX_FALLOFF 0.004f
// Az ameddig az érték a helyén marad képkockákban (60 képkocka / mp)
#define MAX_STAY 30
// A max jelző színe
#define MAX_COLOR 200,200,200

// FFT NE NYÚLJ HOZZÁ
#define FFT_SAMPLE_RATE 40000
#define FFT_SAMPLE      512
#define FFT_RESOLUTION  FFT_SAMPLE_RATE / FFT_SAMPLE
#define FFT_NOISE       500    

arduinoFFT FFT = arduinoFFT();

double vSamp[FFT_SAMPLE];
double vReal[FFT_SAMPLE];
double vImag[FFT_SAMPLE];
int bandValues[LED_COLUMS];

CRGB leds[NUM_LEDS];

int frame = 0;
int brigthness = 0;

int   audio_stay [LED_COLUMS];
float audio_top  [LED_COLUMS];
float audio_level[LED_COLUMS];
float audio_last [LED_COLUMS];

unsigned long millis_        = 0;
unsigned long last_millis_   = 0;
unsigned long button_millis_ = 0;

boolean buttonState = false;
boolean lastButtonState = false;
int ledStyle = 0;
int sensitivity = 1000;

void setup() {
  // ONBOARD LED
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  delay(3000);
  digitalWrite(2, LOW);

  // SERIAL
  #ifdef SERIAL_
  Serial.begin(9600);
  #endif

  // LEDS
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(24);

  // AUDIO
  pinMode(AUDIO_PIN, INPUT);

  // BUTTON
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BUTTON_PIN, HIGH);

  // SENSITIVITY
  pinMode(SENSITIVITY_PIN, INPUT_PULLDOWN);
  digitalWrite(BUTTON_PIN, LOW);
}

float clampMin(float in, int min) {
  if (in < (float)min)
    return min;
  return in;
}

int clampMin(int in, int min) {
  return (in < min) ? min : in;
}

double clamp(double in, double max) {
  if (in > max)
    return max;
  return in;
}

int clamp(int in, int max) {
  while (in < 0)
    in+=max;
  while (in > max)
    in-=max;   
  return in;
}

void handleInput() {
  sensitivity = analogRead(SENSITIVITY_PIN);

  #ifdef SERIAL_
  Serial.printf("%d\n", sensitivity);
  #endif

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState) {
    if (button_millis_ != 1) {
      button_millis_ = millis();
      if (buttonState) {
        ledStyle++;
        button_millis_ = 0;
      }
    }
    else {
      button_millis_ = 0;
    }
    
    lastButtonState = buttonState;
    
    if (ledStyle > 3)
      ledStyle = 0;
  }

  if (button_millis_ > 1 && millis() - button_millis_ > 1000) {
    button_millis_ = 1;
    
    brigthness++;
    if (brigthness > 2)
      brigthness = 0;

    switch (brigthness)
    {
    case 0:
      FastLED.setBrightness(24); 
      break;
    case 1:
      FastLED.setBrightness(127); 
      break;
    case 2:
      FastLED.setBrightness(255); 
      break;
    }
  }
}

void handleAudioInput(int idx) {
  //int audio = clampMin(analogRead(AUDIO_PIN) - AUDIO_MIN, 0);
  int audio = bandValues[idx] / floorl(sensitivity / 200.0 + 1.0);

  audio_last[idx] = audio_level[idx];
  audio_level[idx] = (float)(audio) / (float)(AUDIO_MAX - AUDIO_MIN);

  #ifdef SMOOTHING
  if (audio_last[idx] - audio_level[idx] > SMOOTHNESS_CLAMP)
    audio_level[idx] = audio_last[idx] - SMOOTHNESS_CLAMP;
  #endif

  #ifdef MAX_MARKER
  if (audio_stay[idx] <= 0)
    audio_top[idx] -= MAX_FALLOFF;
  else
    audio_stay[idx]--;
  if (audio_level[idx] > audio_top[idx]) {
    audio_top[idx] = audio_level[idx];
    audio_stay[idx] = MAX_STAY;
  }
  #endif

  #ifdef SERIAL_
  //Serial.printf("%f\n", audio_level);
  #endif
}

void sampleAudio() {
  for (int i = 0; i < FFT_SAMPLE; i++) {
    vSamp[i] = analogRead(AUDIO_PIN);
  }
}

void calculateFFT() {
  for (int i = 0; i < FFT_SAMPLE; i++) {
    vImag[i] = 0.0;
    vReal[i] = vSamp[i];
  }
  
  for (int i = 0; i < LED_COLUMS; i++)
    bandValues[i] = 0;

  FFT.DCRemoval();
  FFT.Windowing(vReal, FFT_SAMPLE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, FFT_SAMPLE, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLE);
  
  // 512 SAMPLE 11 OSZLOP
  /*
  for (int i = 2; i < (FFT_SAMPLE/2); i++){
    if (vReal[i] > FFT_NOISE) {                    
      if (i<=3 )           bandValues[0] += (int)vReal[i];
      if (i>3   && i<=5  ) bandValues[1] += (int)vReal[i];
      if (i>5   && i<=8  ) bandValues[2] += (int)vReal[i];
      if (i>8   && i<=13 ) bandValues[3] += (int)vReal[i];
      if (i>13  && i<=23 ) bandValues[4] += (int)vReal[i];
      if (i>23  && i<=36 ) bandValues[5] += (int)vReal[i];
      if (i>36  && i<=56 ) bandValues[6] += (int)vReal[i];
      if (i>56  && i<=85 ) bandValues[7] += (int)vReal[i];
      if (i>85  && i<=120) bandValues[8] += (int)vReal[i];
      if (i>120 && i<=180) bandValues[9] += (int)vReal[i];
      if (i>180)           bandValues[10]+= (int)vReal[i];
    }
  }
  bandValues[0]  /= 7;
  bandValues[1]  /= 6;
  bandValues[2]  /= 7;
  bandValues[3]  /= 6;
  bandValues[4]  /= 7;
  bandValues[5]  /= 7;
  bandValues[6]  /= 8;
  bandValues[7]  /= 8;
  bandValues[8]  /= 10;
  bandValues[9]  /= 16;
  bandValues[10] /= 21;
  */
  
  // 512 SAMPLE 7 OSZLOP 
  for (int i = 2; i < (FFT_SAMPLE/2); i++){
    if (vReal[i] > FFT_NOISE) {                    
      if (i<=3 )           bandValues[0] += (int)vReal[i];
      if (i>3   && i<=8  ) bandValues[1] += (int)vReal[i];
      if (i>8   && i<=23 ) bandValues[2] += (int)vReal[i];
      if (i>23  && i<=36 ) bandValues[3] += (int)vReal[i];
      if (i>36  && i<=85 ) bandValues[4] += (int)vReal[i];
      if (i>85  && i<=120) bandValues[5] += (int)vReal[i];
      if (i>120)           bandValues[6] += (int)vReal[i];
    }
  }

  bandValues[0]  /= 10;
  bandValues[1]  /= 12;
  bandValues[2]  /= 12;
  bandValues[3]  /= 10;
  bandValues[4]  /= 16;
  bandValues[5]  /= 10;
  bandValues[6]  /= 34;

  //Serial.printf("%d %d %d %d %d %d %d\n\n", bandValues[0], bandValues[1], bandValues[2], bandValues[3], bandValues[4], bandValues[5], bandValues[6]);  
}

void colorLED(int i, int ind) {
  switch (ledStyle) {
    case 0:
      leds[i + ind * LED_ROWS].setHSV(clamp(i*5 + frame, 255), 255, 255);
      break;
    case 1:
      leds[i + ind * LED_ROWS].setHSV(ind*(255.0 / (LED_COLUMS - 1)), 255, 255);
      break;
    case 2:
      leds[i + ind * LED_ROWS] = ColorFromPalette(firePalette, clamp(lround((i / (double)(LED_ROWS)) * 255.0 + (1.0 - clamp(audio_level[ind] * 1.0, 1.0)) * 255.0), 255));
      break;
    case 3:
      leds[i + ind * LED_ROWS] = ColorFromPalette(toxicPalette, round((i / (double)(LED_ROWS - 1.0)) * 255.0));
      break;
  }
}

void loop() {
  sampleAudio();
  handleInput();

  millis_ = millis();
  if (millis_ - last_millis_ > (1000 / FRAMERATE)) {
    calculateFFT();
    for (int ind = 0; ind < LED_COLUMS; ind++) {
      handleAudioInput(ind);

      for (int i = 0; i < LED_ROWS; i++) {
        if (clampMin(((float)(i + 1) / (float)LED_ROWS), 0) <= audio_level[ind])
          colorLED(i, ind);
        else
          leds[i+ ind * LED_ROWS].setRGB(0, 0, 0);
      }

      #ifdef MAX_MARKER
      if (ledStyle != 2) {
        int indx = (int)((float)LED_ROWS * audio_top[ind]) - 1;
        if (indx >= LED_ROWS)
          indx = LED_ROWS - 1;
        if (indx >= 0)
          leds[indx + ind * LED_ROWS].setRGB(MAX_COLOR);
      }
      #endif

      if (ind % 2 == 1) {
        CRGB temp[LED_ROWS];
        for (int i = 0; i < LED_ROWS; i++)
          temp[i] = leds[i + ind * LED_ROWS];
        for (int i = 0; i < LED_ROWS; i++)
          leds[i + ind * LED_ROWS] = temp[LED_ROWS - i - 1];
      }
    }
    FastLED.show();

    frame++;
    if (frame > 255)
      frame = 0;

    last_millis_ = millis_;
  }
}