#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "FastLED.h"

#define NUM_LEDS 12
#define DATA_PIN 5
#define PERIOD  5000
#define ANIMATION_TIME 5000
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0

#define RUN_FOR_N_MILLISECONDS(N)   for(uint32_t start = millis(); (millis()- start) < N; )


/* Handshaking */
const byte SYNC_PKT   =      'A'; // send sync request
const byte SYNC_ACK  =      'B'; // got reply from reciver
const byte ACK       =      'C'; // send final ack
const byte CONN_ESTABLISHED = 'D'; // upon recieving this blink led

bool is_connected = false;
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };

RF24 radio(7, 8);
CRGB leds[NUM_LEDS];

void setup() {  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.clear(true);
  
  set_wdt();

  radio.begin();
  radio.setAutoAck(1);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setRetries(15, 15);
  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(1);
  radio.setCRCLength(RF24_CRC_8 );
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.stopListening();
}

void loop() {
  radio.powerUp();
  delay(100);
  burst_transmit();
  if (is_connected) {
    heart_effect();
    heartbeat();
    is_connected = false;
  }
  else {
    radio.powerUp();
    goto_sleep();
    delay(50);
  }
}

void burst_transmit() {
  is_connected = false;
  byte read_ack  = 0;
  byte send_ack  = SYNC_PKT;

  RUN_FOR_N_MILLISECONDS(PERIOD) {
    if (radio.write(&send_ack, sizeof(send_ack))) {
      send_ack = SYNC_PKT;
      delay(100);
    }
    else {
      while (radio.available()) {
        radio.read(&read_ack, 1);
        if (read_ack == SYNC_ACK) {
          send_ack = ACK;
        }
        if (read_ack == CONN_ESTABLISHED) {
          is_connected = true;
          break;
        }
      }
    }
    if (is_connected) break;
  }
}

void heart_effect() {

  int left_led;
  int right_led;
  FastLED.setBrightness(MAX_BRIGHTNESS);

  for (left_led = 0, right_led = 11; left_led <= 6 && right_led >= 6; left_led++, right_led--) {
    leds[left_led] = CRGB::Red;
    leds[right_led] = CRGB::Red;
    FastLED.show();
    delay(90);
  }
  delay(1000);
  FastLED.clear(true);
  FastLED.show();
}

void heartbeat() {

  int curr_steps = 0;
  int step_incr = 8;

  FastLED.clear(true);
  FastLED.show();
  RUN_FOR_N_MILLISECONDS(ANIMATION_TIME) {

    for (uint8_t i = 0; i < 12; i++) {
      leds[i] = CRGB::Red;
    }
    FastLED.setBrightness(curr_steps);
    FastLED.show();
    delay(45);
    curr_steps += step_incr;

    if (curr_steps > MAX_BRIGHTNESS) {
      curr_steps = MAX_BRIGHTNESS;
      step_incr *= -1;
    }
    else if ( curr_steps < MIN_BRIGHTNESS) {
      curr_steps = MIN_BRIGHTNESS;
      step_incr *= -1;
    }
  }
  FastLED.clear(true); 
  FastLED.show(); 
}

void set_wdt() {
  cli();
  MCUSR &= ~(1 << WDRF);
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = 1 << WDP3; // sleep for 4 seconds
  sei();
}

void goto_sleep() {
  WDTCSR |= _BV(WDIE);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  sleep_disable();
}

ISR(WDT_vect) { }