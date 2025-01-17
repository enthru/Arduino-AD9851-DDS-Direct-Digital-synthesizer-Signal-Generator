#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display init
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool needDisplay = true;

//DDS pins
byte dds_RESET = 6;
byte dds_DATA  = 7;
byte dds_LOAD  = 8;
byte dds_CLOCK = 9;

const unsigned long max_frequency_step = 1000000; //Max Frequency step
const unsigned long max_frequency = 50000000; //Max Frequency
const int min_frequency=25; // Minimum Frequency

unsigned long last_frequency = 5000;
unsigned long frequency_step = 100000;

// Rotary encoder

const int EncoderPinCLK = 2; 
const int EncoderPinDT = 3;  
const int EncoderPinSW = 4;  

// Updated by the ISR (Interrupt Service Routine)
unsigned volatile long frequency = 5000;

// Double click stuff
unsigned long lastPressTime = 0;
const int doubleClickInterval = 300;
bool waitingForSecondClick = false;

// Long click stuff
const int longPressDuration = 1000;
const int delayAfterLongClick = 1000;
bool longPressDetected = false;
unsigned long longClickTime = 0;

// Sweep mode
bool sweepMode = false;
unsigned volatile long sweepStartFrequency =  1000000;
unsigned volatile long sweepStopFrequency =  2000000;
unsigned volatile long sweepCurrentFrequency =  1000001;

unsigned int sweepStep = 1;

unsigned int sweepPoints = 10000;
bool sweepModeStart = true;
bool sweepModeStop = false;
bool sweepModeStep = false;
// sweepMenu is to determine which parameter we are changing in sweep mode
// 1 - Start freq.
// 2 - Stop freq.
// 3 - Sweep points
short sweepMenu = 0;


void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5) {
    if (!sweepMode) {
      if (digitalRead(EncoderPinDT) == LOW)
      {
        frequency=frequency-frequency_step; // Could be -5 or -10
      }
      else {
        frequency=frequency+frequency_step; // Could be +5 or +10
      }

      frequency = min(max_frequency, max(min_frequency, frequency));

      lastInterruptTime = interruptTime;
    } else { 
      switch (sweepMenu) {
        case 1:
          if (digitalRead(EncoderPinDT) == LOW) {
            sweepStartFrequency = sweepStartFrequency - frequency_step;
          } else {
            sweepStartFrequency = sweepStartFrequency + frequency_step;
          }
          sweepStartFrequency = min(max_frequency, max(min_frequency, sweepStartFrequency));
          if (sweepStartFrequency > sweepStopFrequency) sweepStopFrequency = sweepStartFrequency;
          sweepStep = (sweepStopFrequency - sweepStartFrequency)/sweepPoints;
          if (sweepStep == 0) sweepStep = 1;
          sweepCurrentFrequency = sweepStartFrequency;
          Serial.println("Sweep start freq.:");
          Serial.println(format_frequency(sweepStartFrequency));
          Serial.println(sweepStep);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Start:");
          display.setCursor(0, 24);
          display.print(format_frequency(sweepStartFrequency));
          needDisplay = true;
          break;
        case 2:
          if (digitalRead(EncoderPinDT) == LOW) {
            sweepStopFrequency = sweepStopFrequency - frequency_step;
          } else {
            sweepStopFrequency = sweepStopFrequency + frequency_step;
          }
          sweepStopFrequency = min(max_frequency, max(min_frequency, sweepStopFrequency));
          if (sweepStopFrequency < sweepStartFrequency) sweepStartFrequency = sweepStopFrequency;
          sweepStep = (sweepStopFrequency - sweepStartFrequency)/sweepPoints;
          if (sweepStep == 0) sweepStep = 1;
          sweepCurrentFrequency = sweepStartFrequency;
          Serial.println("Sweep stop freq.:");
          Serial.println(format_frequency(sweepStopFrequency));
          Serial.println(sweepStep);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Stop:");
          display.setCursor(0, 24);
          display.print(format_frequency(sweepStopFrequency));
          needDisplay = true;
          break;
        case 3:
          if (digitalRead(EncoderPinDT) == LOW) {
            if (sweepPoints > 1000) sweepPoints = sweepPoints - 1000;
          } else {
            if (sweepPoints <= 49100) sweepPoints = sweepPoints + 1000;
          }
          sweepStep = (sweepStopFrequency - sweepStartFrequency)/sweepPoints;
          if (sweepStep == 0) sweepStep = 1;
          sweepCurrentFrequency = sweepStartFrequency;
          Serial.println("Sweep points:");
          Serial.println(sweepPoints);
          Serial.println(sweepStep);
          display.clearDisplay();
          display.setCursor(0, 0);
          display.print("Points:");
          display.setCursor(0, 24);
          display.print(sweepPoints);
          needDisplay = true;
          break;
      }
      lastInterruptTime = interruptTime; 
    }
  }
}

void show_frequency()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(format_frequency(frequency));
  display.display();
}

#include <stdio.h>
#include <string.h>

#include <stdio.h>
#include <string.h>
 
char* format_frequency(unsigned volatile long frequency) {
    static char output[50];

    if (frequency >= 1000000) {
        unsigned long mhz = frequency / 1000000;
        unsigned long khz = (frequency % 1000000) / 1000;
        unsigned long hz = frequency % 1000;
        sprintf(output, "%lu.%03lu.%03lu MHz", mhz, khz, hz);
    } else if (frequency >= 1000) {
        unsigned long khz = frequency / 1000;
        unsigned long hz = frequency % 1000;
        sprintf(output, "%lu.%03lu kHz", khz, hz);
    } else {
        sprintf(output, "%lu Hz", frequency);
    }

    return output;
}

void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(100);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  // Rotary pulses are INPUTs
  pinMode(EncoderPinCLK, INPUT);
  pinMode(EncoderPinDT, INPUT);

  // Switch is floating so use the in-built PULLUP so we don't need a resistor
  pinMode(EncoderPinSW, INPUT_PULLUP);

  // Attach the routine to service the interrupts
  attachInterrupt(digitalPinToInterrupt(EncoderPinCLK), isr, LOW);
  setup_dds();

  show_frequency();
  display.display();
  dds(frequency);
  Serial.println("Start");
}

void loop() {

  //Encoder pressed
  if (!digitalRead(EncoderPinSW)) {
    unsigned long currentTime = millis();

    if (currentTime - longClickTime > delayAfterLongClick) {
      longPressDetected = false;
      Serial.println("Long press = false");
    }
    
    while (!digitalRead(EncoderPinSW)) {
      if (millis() - currentTime >= longPressDuration) {
        longClickTime = millis();
        longPressDetected = true;
        Serial.println("Long press = true");
        if (sweepMenu !=3) sweepMenu++; else sweepMenu = 1;
          Serial.println("Sweep menu item:");
          Serial.println(sweepMenu);
          waitingForSecondClick = false;
          switch(sweepMenu){
            case 1:
              display.clearDisplay();
              display.setCursor(0, 0);
              display.print("Start:");
              display.setCursor(0, 24);
              display.print(format_frequency(sweepStartFrequency));
              display.display();
              break;
            case 2:
              display.clearDisplay();
              display.setCursor(0, 0);
              display.print("Stop:");
              display.setCursor(0, 24);
              display.print(format_frequency(sweepStopFrequency));
              display.display();
              break;
            case 3:
              display.clearDisplay();
              display.setCursor(0, 0);
              display.print("Points:");
              display.setCursor(0, 24);
              display.print(sweepPoints);
              display.display();
              break;
          }
        break;
      }
      delay(10);
    }

    if (waitingForSecondClick && (currentTime - lastPressTime <= doubleClickInterval)) {
      if (!sweepMode) { 
        Serial.println("Sweep mode");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Sweeping");
        display.display();
        sweepMode = true; 
      } else {
        sweepMode = false;
        Serial.println("Generator mode");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print(frequency);
        display.display();
      }
      waitingForSecondClick = false;
    } else {
      waitingForSecondClick = true;
      lastPressTime = currentTime;
    }
  }

  if (waitingForSecondClick && (millis() - lastPressTime > doubleClickInterval) && !longPressDetected) {
    if (frequency_step == max_frequency_step) {
      frequency_step = 1;
    } else {
      frequency_step *= 10;
    }
    Serial.print("Multiplier:");
    Serial.println(frequency_step);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Step:");
    display.setCursor(0, 24);
    display.print(format_frequency(frequency_step));
    display.display();
    waitingForSecondClick = false;
  }
 
  if (!sweepMode) {
    if (frequency != last_frequency) {
      Serial.print(frequency > last_frequency ? "Up  :" : "Down:");
      Serial.println(format_frequency(frequency));
      show_frequency();
      dds(frequency);
      last_frequency = frequency;
    }
  } else {
    if (needDisplay) {
      display.display();
      needDisplay = false;
    }
    if (sweepCurrentFrequency >= sweepStopFrequency) sweepCurrentFrequency = sweepStartFrequency;
    sweepCurrentFrequency = sweepCurrentFrequency + sweepStep;
    dds(sweepCurrentFrequency);
  }
}
