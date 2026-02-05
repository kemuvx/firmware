#ifndef LITE_VERSION
#include "ibutton.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <OneWire.h>
#include <array>
#include <string.h>
#define ONE_WIRE_BUS 0

namespace {
enum class state { READING, WRITING };
auto currentState = state::READING;

byte idBuffer[8];
byte currentId[8] = {0};
byte previousId[8] = {0};
bool hasId = false;

int pin = 0; // G0
bool hasError = false;
OneWire *oneWire = nullptr;
bool changedScreen = true;
bool running = true;
} // namespace

void iButtonStateManager() {
    if (oneWire == nullptr) {
        oneWire = new OneWire(pin);
    } else {
        // Reset existing OneWire object
        delete oneWire;
        oneWire = new OneWire(pin);
    }
    while (running) {
        switch (currentState) {
            case state::READING: readingScreen(); break;
            case state::WRITING: writingScreen(); break;
        }
        delay(50);
    }
    memset(currentId, 0, 8);
    memset(previousId, 0, 8);
    memset(idBuffer, 0, 8);
    hasId = false;
    hasError = false;
    changedScreen = true;
    currentState = state::READING;
    running = true;
    returnToMenu = true;
}
bool readOneWire() {
    if (oneWire->reset() != 0) {
        oneWire->write(0x33);
        oneWire->read_bytes(idBuffer, 8);
        return true;
    }
    return false;
}

void readingScreen() {
    bool readRes = readOneWire();
    if (changedScreen) {
        tft.fillScreen(TFT_BLACK);
        drawMainBorderWithTitle("Reading iButton");
        tft.setTextSize(2);
        tft.setCursor(12, 57);
        memset(previousId, 0xFF, 8);
        if (hasId) {
            tft.fillRect(12, 57, 220, 30, TFT_BLACK);
            tft.setCursor(12, 57);

            for (byte i = 0; i < 8; i++) {
                if (currentId[i] < 0x10) tft.print("0");
                tft.print(currentId[i], HEX);
                if (i < 7) tft.print(":");
            }

            memcpy(previousId, currentId, 8);
        } else {
            tft.setCursor(12, 57);
            tft.print("No iButton detected");
            memset(previousId, 0xFF, 8);
        }
        changedScreen = false;
    }

    if (readRes) {
        hasError = OneWire::crc8(idBuffer, 7) != idBuffer[7];
        if (!hasError) {
            memcpy(currentId, idBuffer, 8);
            hasId = true;
            if (memcmp(currentId, previousId, 8)) {
                tft.fillRect(12, 57, 220, 30, TFT_BLACK);
                tft.setCursor(12, 57);
                for (byte i = 0; i < 8; i++) {
                    if (currentId[i] < 0x10) { tft.print("0"); }
                    tft.print(currentId[i], HEX);
                    if (i < 7) tft.print(":");
                }
            }
            memcpy(previousId, currentId, 8);
        }
    }
    if (check(EscPress)) {
        running = false;
        return;
    } else if (check(NextPress)) {
        // TODO: Open id selection
    } else if (check(SelPress)) {
        if (hasId) {
            currentState = state::WRITING;
            changedScreen = true;
            delay(100);
            return;
        }
    }
}

void writingScreen() {
    if (changedScreen) {
        tft.fillScreen(TFT_BLACK);
        drawMainBorderWithTitle("Writing iButton");
        tft.setTextSize(1);
        tft.setCursor(12, 27);
        tft.print("TARGET ");
        tft.setCursor(12, 37);
        for (byte i = 0; i < 8; i++) {
            if (currentId[i] < 0x10) { tft.print("0"); }
            tft.print(currentId[i], HEX);
            if (i < 7) tft.print(":");
        }
        tft.setCursor(12, 57);
        tft.setTextSize(2);
        tft.print("Press next to start");
        changedScreen = false;
    }

    if (check(EscPress)) {
        running = false;
        return;
    } else if (check(NextPress)) {
        writingSearch();
        currentState = state::READING;
        changedScreen = true;
        delay(100);
        return;

    } else if (check(SelPress)) {
        currentState = state::READING;
        changedScreen = true;
        delay(100);
        return;
    }
}
void writingSearch() {
    int found = false;
    while (!found) {
        if (check(SelPress)) {
            currentState = state::READING;
            changedScreen = true;
            return;
        }
        if (check(EscPress)) {
            running = false;
            return;
        }

        bool readRes = readOneWire();
        if (readRes) {
            write_ibutton();
            found = true;
        }

        delay(10);
    }
}
void write_byte_rw1990(byte data) {
    int data_bit;
    for (data_bit = 0; data_bit < 8; data_bit++) {
        if (data & 1) {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            delayMicroseconds(60);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
            pinMode(pin, OUTPUT);
            pinMode(pin, INPUT);
            digitalWrite(pin, HIGH);
        }
        delay(10);
        data = data >> 1;
    }
}
void write_ibutton() {

    tft.setCursor(52, 102);
    tft.print("Wait...");
    tft.setCursor(110, 102);

    tft.print('-');
    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x33); // Read ROM

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x3C);
    tft.print('-');
    delay(50);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD1);
    tft.print('-');
    delay(50);

    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    delayMicroseconds(60);
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
    delay(10);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD5);
    tft.print('-');
    delay(50);
    tft.print('>');
    for (byte i = 0; i < 8; i++) {
        write_byte_rw1990(currentId[i]);
        tft.print('*');
        delayMicroseconds(25);
    }
    oneWire->reset();
    oneWire->skip();

    oneWire->write(0xD1);
    delayMicroseconds(16);
    oneWire->reset();

    tft.setCursor(90, 50);
    tft.setTextSize(FM);
    displayTextLine("COPIED");
    tft.setCursor(40, 80);
    displayTextLine("Press SEL to return");
    while (true) {
        if (check(SelPress)) {
            currentState = state::READING;
            memset(currentId, 0, 8);
            memset(previousId, 0, 8);
            hasError = false;
            hasId = false;
            changedScreen = true;
            return;
        }
        if (check(EscPress)) {
            running = false;
            return;
        }
    }
}
#endif
