#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "SBUS2.h"
#include "SBUS_usart.h"

// NeoPixel -määritykset
#define LED_PIN 6
#define NUM_LEDS 30

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

// SBUS2 -kanavamääritykset (Futaba CH2 = 1, CH6 = 5, CH7 = 6)
#define CH2_INDEX 1 // Kaasu / Jarru
#define CH6_INDEX 5 // YLÖS / Seuraava tila
#define CH7_INDEX 6 // ALAS / Edellinen tila

// =====================================================
// KAASUN JA JARRUN SÄÄDÖT
// =====================================================
// Jos jarruvalot syttyvät kaasuttaessa ja sammuvat jarruttaessa, vaihda 1 -> -1
const int BRAKE_DIRECTION = 1; 

const int CH2_DEADZONE = 50; // Kuollut alue neutraalin ympärillä

int ch2Neutral = 0;           // Kalibroituu automaattisesti käynnistyksessä
bool isCalibrated = false;

// =====================================================
// ASETUKSET
// =====================================================

int lightMode = 1; // 0 = Pois, 1 = Perusvalot, 2 = Poliisivalot
const int MAX_MODES = 3; 

bool prevCh6Pressed = false;
bool prevCh7Pressed = false;

unsigned long lastSignal = 0;
unsigned long lastShow = 0;

// Alustavalot
const int underglowMax = 100;
const int underglowTail = 5;
const int underglowSpeed = 35;

// Poliisiauton valot
const int policeBrightness = 180;
const int policeSpeed = 100;

// Jarruvalot
const int brakeBrightness = 255;

// Pakoputki
const int flameMin = 40;
const int flameMax = 255;


// =====================================================
// VALOFUNKTIOIT
// =====================================================

void setPink(int led, int brightness)
{
  leds.setPixelColor(led, leds.Color(brightness, 0, brightness, 0));
}

// 1. Alustavalot
void updateUnderglow()
{
  static float position = 0;
  static int direction = 1;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate >= underglowSpeed)
  {
    lastUpdate = millis();
    position += direction * 0.35;

    if (position >= 17) { position = 17; direction = -1; }
    if (position <= 0)  { position = 0;  direction = 1;  }
  }

  for (int i = 0; i < 18; i++)
  {
    float distance = abs(i - position);

    if (distance <= underglowTail)
    {
      float brightness = underglowMax - (distance * (underglowMax / underglowTail));
      if (brightness < 0) brightness = 0;
      setPink(i, brightness);
    }
    else
    {
      leds.setPixelColor(i, 0);
    }
  }
}

// 2. Jarruvalot (Pimeänä vapaalla ja kaasulla, syttyvät vain jarruttaessa)
void updateBrakeLights(bool isBraking)
{
  for (int i = 18; i <= 24; i++)
  {
    if (isBraking)
    {
      leds.setPixelColor(i, leds.Color(brakeBrightness, 0, 0, 0));
    }
    else
    {
      leds.setPixelColor(i, 0);
    }
  }
}

// 3. Poliisivalot
void setPoliceLights()
{
  static unsigned long lastChange = 0;
  static bool state = false;

  if (millis() - lastChange >= policeSpeed)
  {
    lastChange = millis();
    state = !state;
  }

  if (state)
  {
    leds.setPixelColor(25, leds.Color(policeBrightness, 0, 0, 0));
    leds.setPixelColor(26, leds.Color(policeBrightness, 0, 0, 0));
    leds.setPixelColor(27, leds.Color(0, 0, policeBrightness, 0));
    leds.setPixelColor(28, leds.Color(0, 0, policeBrightness, 0));
  }
  else
  {
    leds.setPixelColor(25, leds.Color(0, 0, policeBrightness, 0));
    leds.setPixelColor(26, leds.Color(0, 0, policeBrightness, 0));
    leds.setPixelColor(27, leds.Color(policeBrightness, 0, 0, 0));
    leds.setPixelColor(28, leds.Color(policeBrightness, 0, 0, 0));
  }
}

// 4. Pakoputken liekki
void updateFlame()
{
  static unsigned long lastUpdate = 0;
  static int flame = flameMin;

  if (millis() - lastUpdate >= 30)
  {
    lastUpdate = millis();
    flame = random(flameMin, flameMax + 1);
  }

  leds.setPixelColor(29, leds.Color(flame, flame * 0.30, 0, 0));
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  leds.begin();
  leds.setBrightness(255);
  leds.clear();
  leds.show();

  randomSeed(analogRead(A0));

  // Syötetään 5V D2-pinnistä inverteripiirille
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);

  SBUS2_Setup();
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  static bool isBraking = false;

  // 1. Lue S.BUS2-dataa
  if (SBUS_Ready())
  {
    int16_t ch2 = SBUS2_get_servo_data(CH2_INDEX);
    int16_t ch6 = SBUS2_get_servo_data(CH6_INDEX);
    int16_t ch7 = SBUS2_get_servo_data(CH7_INDEX);

    if (ch2 > 0 && ch6 >= 0 && ch7 >= 0)
    {
      lastSignal = millis();

      // Automaattinen keskikohdan tallennus käynnistyksessä (älä koske liipaisimeen kun kytket virrat)
      if (!isCalibrated)
      {
        ch2Neutral = ch2;
        isCalibrated = true;
      }

      // Lasketaan poikkeama keskikohdasta
      int delta = (ch2 - ch2Neutral) * BRAKE_DIRECTION;

      // Jarrutus on päällä vain kun poikkeama ylittää kynnysarvon jarrun suuntaan
      isBraking = (delta > CH2_DEADZONE);

      // Tilanvaihto CH6 / CH7
      bool ch6Pressed = (ch6 >= 1300);
      bool ch7Pressed = (ch7 >= 1300);

      if (ch6Pressed && !prevCh6Pressed)
      {
        lightMode++;
        if (lightMode >= MAX_MODES) lightMode = 0;
      }

      if (ch7Pressed && !prevCh7Pressed)
      {
        lightMode--;
        if (lightMode < 0) lightMode = MAX_MODES - 1;
      }

      prevCh6Pressed = ch6Pressed;
      prevCh7Pressed = ch7Pressed;
    }
  }

  // 2. Ohjaa valoja
  if (millis() - lastShow >= 15)
  {
    lastShow = millis();

    if (millis() - lastSignal < 1000)
    {
      switch (lightMode)
      {
        case 0: // Pois
          for (int i = 0; i < 18; i++) leds.setPixelColor(i, 0);
          for (int i = 25; i <= 29; i++) leds.setPixelColor(i, 0);
          break;

        case 1: // Perustila (Alustavalot + Liekki)
          for (int i = 25; i <= 28; i++) leds.setPixelColor(i, 0);
          updateUnderglow();
          updateFlame();
          break;

        case 2: // Poliisitila (Poliisivalot + Liekki)
          for (int i = 0; i < 18; i++) leds.setPixelColor(i, 0);
          setPoliceLights();
          updateFlame();
          break;
      }

      // Jarruvalot toimivat kaikissa tiloissa
      updateBrakeLights(isBraking);
    }
    else
    {
      leds.clear();
    }

    leds.show();
  }
}