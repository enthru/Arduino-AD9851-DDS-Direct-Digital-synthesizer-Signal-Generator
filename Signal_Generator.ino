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

//DDS pins
byte dds_RESET = 6;
byte dds_DATA  = 7;
byte dds_LOAD  = 8;
byte dds_CLOCK = 9;

const unsigned long max_frequency_step = 1000000; //Max Frequency step
const unsigned long max_frequency = 50000000; //Max Frequency
const int min_frequency=25; // Minimum Frequency

unsigned long last_frequency = 5000;
unsigned long frequency_step = 1;

// Rotary encoder

const int EncoderPinCLK = 2; 
const int EncoderPinDT = 3;  
const int EncoderPinSW = 4;  

// Updated by the ISR (Interrupt Service Routine)
unsigned volatile long frequency = 5000;

void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(EncoderPinDT) == LOW)
    {
      frequency=frequency-frequency_step ; // Could be -5 or -10
    }
    else {
      frequency=frequency+frequency_step ; // Could be +5 or +10
    }

    frequency = min(max_frequency, max(min_frequency, frequency));

    lastInterruptTime = interruptTime;
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
  delay(2000);
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
  // Is someone pressing the rotary switch?
  if ((!digitalRead(EncoderPinSW))) {
    while (!digitalRead(EncoderPinSW))
      delay(10);
    Serial.println("Reset");
    if (frequency_step==max_frequency_step)
    {
      frequency_step=1;
    }
    else
    {
      frequency_step=frequency_step*10;  
    }
    Serial.print("multiplier:");
    Serial.println(frequency_step);
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Step:");
    display.setCursor(0,24);
    display.print(format_frequency(frequency_step));
    display.display();
  }
 
  if (frequency != last_frequency) {
    Serial.print(frequency > last_frequency ? "Up  :" : "Down:");
    Serial.println(format_frequency(frequency));
    show_frequency();
    dds(frequency);
    last_frequency = frequency ;
  }
}
