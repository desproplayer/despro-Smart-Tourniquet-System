#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "state_machine.h"

class UIDriver {
public:
    UIDriver() : display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1) {}

    bool begin() {
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(LED_GREEN_PIN, OUTPUT);
        pinMode(LED_YELLOW_PIN, OUTPUT);
        pinMode(LED_RED_PIN, OUTPUT);
        allLedsOff();

        if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
            return false;
        }
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        return true;
    }

    void showSelfCheck() {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Smart Tourniquet");
        display.println("Self-check...");
        display.display();
        setLed(LED_GREEN_PIN, true);
    }

    void showPressureStatus(float pressureMmHg, TourniquetState state, unsigned long elapsedSec) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("TEKANAN:");

        display.setTextSize(2);
        display.setCursor(0, 12);
        display.print(pressureMmHg, 0);
        display.println(" mmHg");

        display.setTextSize(1);
        display.setCursor(0, 34);
        display.print("Status: ");
        display.println(stateToString(state));

        display.setCursor(0, 46);
        unsigned long mm = elapsedSec / 60;
        unsigned long ss = elapsedSec % 60;
        display.printf("Waktu: %02lu:%02lu\n", mm, ss);

        display.display();
    }

    void showMessage(const char* line1, const char* line2 = "") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 20);
        display.println(line1);
        display.setCursor(0, 34);
        display.println(line2);
        display.display();
    }

    // --- LED ---
    void setStatusOK() {
        allLedsOff();
        setLed(LED_GREEN_PIN, true);
    }

    void setStatusWarning() {
        allLedsOff();
        setLed(LED_YELLOW_PIN, true);
    }

    void setStatusCritical() {
        allLedsOff();
        setLed(LED_RED_PIN, true);
    }

    void blinkCriticalLed() {
        static unsigned long lastToggle = 0;
        static bool ledState = false;
        if (millis() - lastToggle > 200) { // berkedip cepat sesuai proposal (kondisi bocor)
            ledState = !ledState;
            digitalWrite(LED_RED_PIN, ledState);
            lastToggle = millis();
        }
    }

    // --- Buzzer ---
    void buzzerBeepShort() {
        tone(BUZZER_PIN, 2400, 150); // 2.4kHz sesuai spek proposal
    }

    void buzzerAlarmContinuous() {
        tone(BUZZER_PIN, 2400);
    }

    void buzzerOff() {
        noTone(BUZZER_PIN);
    }

private:
    Adafruit_SSD1306 display;

    void allLedsOff() {
        digitalWrite(LED_GREEN_PIN, LOW);
        digitalWrite(LED_YELLOW_PIN, LOW);
        digitalWrite(LED_RED_PIN, LOW);
    }

    void setLed(int pin, bool on) {
        digitalWrite(pin, on ? HIGH : LOW);
    }
};
