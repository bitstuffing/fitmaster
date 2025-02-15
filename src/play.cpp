#include "play.h"
#include "hardware.h"

#include <Arduino.h>

enum State { WAITING, BEEPING, COUNTDOWN, TOUCHED };
State currentState = WAITING;
unsigned long lastUpdate = 0;
int countdownStep = 0;

void beep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

void playLoop() {
  digitalWrite(LED_BUILTIN_PIN, digitalRead(TOUCH_PIN)); 

  switch (currentState) {
    case WAITING:
      if (millis() > 2000) {
        currentState = BEEPING;
        lastUpdate = millis();
        beep();
        pixels.fill(pixels.Color(0, 255, 0));
        pixels.show();
        currentState = COUNTDOWN;
      }
      break;

    case COUNTDOWN:
      if (millis() - lastUpdate >= 1000) {
        if (countdownStep < NUM_PIXELS) {
          if (countdownStep > 0) {
            pixels.setPixelColor(countdownStep - 1, pixels.Color(0, 0, 0));
          }
          pixels.setPixelColor(countdownStep, pixels.Color(255, 0, 0));
          pixels.show();
          countdownStep++;
          lastUpdate = millis();
        }else if(countdownStep == NUM_PIXELS){
          pixels.fill(pixels.Color(255, 0, 0));
          pixels.show();
        //   currentState = WAITING;
          lastUpdate = millis();
        }
      }

      if (digitalRead(TOUCH_PIN) == HIGH) { 
        currentState = TOUCHED;
        beep();
        pixels.fill(0);
        pixels.show();
        countdownStep = 0;
        lastUpdate = millis();
      }
      break;

    case TOUCHED:
      if (millis() - lastUpdate > 1000) {
        currentState = WAITING;
      }
      break;
  }
}