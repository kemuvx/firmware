#ifndef LITE_VERSION
#include "ibutton.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <array>
#define ONE_WIRE_BUS 0

OneWire *oneWire = nullptr;
byte buffer[8];

namespace {
enum class state { SETUP, READING, WRITING };
auto currentState = state::SETUP;

std::array<byte, 8> currentId;
bool hasId = false;

int pin = 0; // G0
bool hasPin = true;
bool hasError = false;
bool screenNeedsUpdate = true;
} // namespace

void iButtonStateManager() {
    if (oneWire == nullptr) { oneWire = new OneWire(pin); }
    while (!returnToMenu) {
        if (screenNeedsUpdate) {
            tft.fillScreen(TFT_BLACK);
            screenNeedsUpdate = false;
        }
        switch (currentState) {
            case state::SETUP: setupScreen(); break;
            case state::READING: readingScreen(); break;
            case state::WRITING: writingScreen(); break;
        }
        delay(250);
    }
    if (oneWire) {
        delete oneWire;
        oneWire = nullptr;
    }
    currentState = state::SETUP;
    hasId = false;
}
void setupScreen() {
    /*TODO*/
    currentState = state::READING;
    screenNeedsUpdate = true;
}
void readingScreen() {
    if (check(EscPress)) {
        returnToMenu = true;
        return;
    } else if (check(NextPress)) {
        // TODO: Open id selection
    } else if (check(SelPress)) {
        if (hasId) {
            currentState = state::WRITING;
            screenNeedsUpdate = true;
            return;
        }
    }

    drawMainBorderWithTitle("Reading iButton");
    tft.setTextSize(1.75);
    tft.setCursor(12, 57);

    if (oneWire->reset() != 0) {
        oneWire->write(0x33);
        oneWire->read_bytes(buffer, 8);

        hasError = OneWire::crc8(buffer, 7) != buffer[7];
        if (!hasError) {
            hasId = true;
            std::array<byte, 8> newId = std::to_array(buffer);
            if (currentId != newId) {
                currentId = newId;
                screenNeedsUpdate = true;
            }
        }
    }

    if (!hasId) {
        tft.print("No iButton detected");
    } else {
        for (byte i = 0; i < 8; i++) {
            if (currentId[i] < 0x10) { tft.print("0"); }
            tft.print(currentId[i], HEX);
            if (i < 7) tft.print(":");
        }
    }
}

void writingScreen() {
    if (check(EscPress)) {
        returnToMenu = true;
        return;
    } else if (check(NextPress)) {
        // TODO: Open id selection
    } else if (check(SelPress)) {
        if (hasId) {
            currentState = state::READING;
            screenNeedsUpdate = true;
            return;
        }
    }

    drawMainBorderWithTitle("Writing iButton");
    tft.setTextSize(1.75);
    tft.setCursor(12, 57);
    tft.print("Writing ");
    for (byte i = 0; i < 8; i++) {
        if (currentId[i] < 0x10) { tft.print("0"); }
        tft.print(currentId[i], HEX);
        if (i < 7) tft.print(":");
    }
}
void write_byte_rw1990(byte data) {
    int data_bit;
    uint8_t pin = bruceConfigPins.iButton;
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

    // Dislay ID
    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton Write");
    tft.setCursor(11, 50);
    tft.print("Current buffer:");
    tft.setCursor(40, 57);
    for (byte i = 0; i < 8; i++) {
        tft.print(buffer[i], HEX);
        tft.print(":");
    }
    delay(1000);
    tft.setCursor(52, 102);
    tft.print("Wait...");
    tft.setCursor(110, 102);

    tft.print('-');
    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x33); // Read ROM

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0x3C); // Set write mode for some models
    tft.print('-');
    delay(50);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD1); // Write command
    tft.print('-');
    delay(50);

    // Write don't work without this code
    uint8_t pin = bruceConfigPins.iButton;
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    delayMicroseconds(60);
    pinMode(pin, INPUT);
    digitalWrite(pin, HIGH);
    delay(10);

    oneWire->skip();
    oneWire->reset();
    oneWire->write(0xD5); // Enter write mode
    tft.print('-');
    delay(50);
    tft.print('>');
    for (byte i = 0; i < 8; i++) {
        write_byte_rw1990(buffer[i]); // Write each byte
        tft.print('*');
        delayMicroseconds(25);
    }
    oneWire->reset(); // Reset bus
    oneWire->skip();

    // Step 3 : Finalise
    oneWire->write(0xD1); // End of write command
    delayMicroseconds(16);
    oneWire->reset(); // Reset bus

    // Display end of copy
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(90, 50);
    tft.setTextSize(FM);
    displayTextLine("COPIED");
    tft.setCursor(40, 80);
    tft.print("Release button");

    delay(3000);

    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton");
    tft.setCursor(10, 60);
    displayTextLine("Waiting iButton...");
}

void read_ibutton() {
    oneWire->write(0x33);           // Read ID command
    oneWire->read_bytes(buffer, 8); // Read ID

    // Display iButton
    tft.fillScreen(TFT_BLACK);
    drawMainBorderWithTitle("iButton ID");

    // Dislay ID
    tft.setTextSize(1.75);
    tft.setCursor(12, 57);
    for (byte i = 0; i < 8; i++) {
        tft.print(buffer[i], HEX);
        tft.print(":");
    }

    if (OneWire::crc8(buffer, 7) != buffer[7]) {
        tft.setCursor(55, 85);
        tft.setTextSize(FM);
        tft.setTextColor(TFT_RED);
        tft.println("CRC ERROR!");
    } else {
        // Display copy infos
        tft.setCursor(55, 85);
        tft.setTextSize(1.5);
        tft.println("Hold OK to copy");
    }
}

#endif
