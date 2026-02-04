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
        tft.setTextSize(2);
        tft.setCursor(12, 57);
        tft.print("Writing ");
        tft.setCursor(12, 77);
        for (byte i = 0; i < 8; i++) {
            if (currentId[i] < 0x10) { tft.print("0"); }
            tft.print(currentId[i], HEX);
            if (i < 7) tft.print(":");
        }
        changedScreen = false;
    }
    if (check(EscPress)) {
        running = false;
        return;
    } else if (check(NextPress)) {
        // TODO: Open id selection
    } else if (check(SelPress)) {
        currentState = state::READING;
        changedScreen = true;
        delay(100);
        return;
    }
}

// void write_byte_rw1990(byte data) {
//     int data_bit;
//     uint8_t pin = bruceConfigPins.iButton;
//     for (data_bit = 0; data_bit < 8; data_bit++) {
//         if (data & 1) {
//             digitalWrite(pin, LOW);
//             pinMode(pin, OUTPUT);
//             delayMicroseconds(60);
//             pinMode(pin, INPUT);
//             digitalWrite(pin, HIGH);
//         } else {
//             digitalWrite(pin, LOW);
//             pinMode(pin, OUTPUT);
//             pinMode(pin, INPUT);
//             digitalWrite(pin, HIGH);
//         }
//         delay(10);
//         data = data >> 1;
//     }
// }
// void write_ibutton() {
//     // Dislay ID
//     tft.fillScreen(TFT_BLACK);
//     drawMainBorderWithTitle("iButton Write");
//     tft.setCursor(11, 50);
//     tft.print("Current idBuffer:");
//     tft.setCursor(40, 57);
//     for (byte i = 0; i < 8; i++) {
//         tft.print(idBuffer[i], HEX);
//         tft.print(":");
//     }
//     delay(1000);
//     tft.setCursor(52, 102);
//     tft.print("Wait...");
//     tft.setCursor(110, 102);

//     tft.print('-');
//     oneWire->skip();
//     oneWire->reset();
//     oneWire->write(0x33); // Read ROM

//     oneWire->skip();
//     oneWire->reset();
//     oneWire->write(0x3C); // Set write mode for some models
//     tft.print('-');
//     delay(50);

//     oneWire->skip();
//     oneWire->reset();
//     oneWire->write(0xD1); // Write command
//     tft.print('-');
//     delay(50);

//     // Write don't work without this code
//     uint8_t pin = bruceConfigPins.iButton;
//     digitalWrite(pin, LOW);
//     pinMode(pin, OUTPUT);
//     delayMicroseconds(60);
//     pinMode(pin, INPUT);
//     digitalWrite(pin, HIGH);
//     delay(10);

//     oneWire->skip();
//     oneWire->reset();
//     oneWire->write(0xD5); // Enter write mode
//     tft.print('-');
//     delay(50);
//     tft.print('>');
//     for (byte i = 0; i < 8; i++) {
//         write_byte_rw1990(idBuffer[i]); // Write each byte
//         tft.print('*');
//         delayMicroseconds(25);
//     }
//     oneWire->reset(); // Reset bus
//     oneWire->skip();

//     // Step 3 : Finalise
//     oneWire->write(0xD1); // End of write command
//     delayMicroseconds(16);
//     oneWire->reset(); // Reset bus

//     // Display end of copy
//     tft.fillScreen(TFT_BLACK);
//     tft.setCursor(90, 50);
//     tft.setTextSize(FM);
//     displayTextLine("COPIED");
//     tft.setCursor(40, 80);
//     tft.print("Release button");

//     delay(3000);

//     tft.fillScreen(TFT_BLACK);
//     drawMainBorderWithTitle("iButton");
//     tft.setCursor(10, 60);
//     displayTextLine("Waiting iButton...");
// }

// void read_ibutton() {
//     oneWire->write(0x33);             // Read ID command
//     oneWire->read_bytes(idBuffer, 8); // Read ID

//     // Display iButton
//     tft.fillScreen(TFT_BLACK);
//     drawMainBorderWithTitle("iButton ID");

//     // Dislay ID
//     tft.setTextSize(1.75);
//     tft.setCursor(12, 57);
//     for (byte i = 0; i < 8; i++) {
//         tft.print(idBuffer[i], HEX);
//         tft.print(":");
//     }

//     if (OneWire::crc8(idBuffer, 7) != idBuffer[7]) {
//         tft.setCursor(55, 85);
//         tft.setTextSize(FM);
//         tft.setTextColor(TFT_RED);
//         tft.println("CRC ERROR!");
//     } else {
//         // Display copy infos
//         tft.setCursor(55, 85);
//         tft.setTextSize(1.5);
//         tft.println("Hold OK to copy");
//     }
// }

#endif
