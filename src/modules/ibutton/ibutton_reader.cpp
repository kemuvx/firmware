#include "OneWire.h"
#include "core/display.h"
#include "core/menu_items/iButtonMenu.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include "ibutton_helper.h"
#include <iostream>
#include <utility>
#include <vector>

// ========== FORWARD DECLARATIONS ==========
namespace {
// Drawing functions
void draw_ibutton_saved();
void draw_ibutton_save_failed();
void draw_ibutton_id(String id);
void draw_ibutton_crc_error(byte crc, byte expected_crc);
void draw_black_over_crc_error();
void draw_button_help();
void draw_read();
void draw_already_saved();
void draw_no_ibutton();
void draw_no_ibutton_error();
void draw_black_over_id();

// iButton operations
void init_onewire();
void cleanup_onewire();
void cleanup_variables();
bool read_rom(byte *buffer);
byte get_crc8(const byte *data, uint8_t length);

// Array operations
void add_to_ibuttons_array(const String name, const std::array<byte, 8> &id);

// Saved iButton checking
bool check_id_already_saved();

// File operations
void append_ibutton_id_to_file(const String name, const String formattedId);
// User input
String get_name_from_keyboard();
void handle_input();
void delay_with_input_handling(unsigned long ms);
// ==========================================
} // namespace
namespace {
std::vector<std::pair<String, std::array<byte, 8>>> saved_ibuttons = {};
byte id_buffer[8];
byte current_id[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte previous_id[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool has_id = false;
bool previous_has_error = false;
bool has_error = false;
bool crc_changed = false;
byte last_crc = 0xFF;
byte last_expected_crc = 0xFF;
byte current_crc;
OneWire *one_wire = nullptr;
} // namespace
namespace {
void init_onewire() {
    if (one_wire == nullptr) { one_wire = new OneWire(IBUTTON_PIN); }
}

void cleanup_onewire() {
    if (one_wire != nullptr) {
        delete one_wire;
        one_wire = nullptr;
    }
}
void cleanup_variables() {
    has_id = false;
    previous_has_error = false;
    has_error = false;
    last_crc = 0xFF;
    last_expected_crc = 0xFF;
    saved_ibuttons.clear();
    memset(current_id, 0x00, 8);
    memset(previous_id, 0xFF, 8);
    memset(id_buffer, 0, 8);
}
String get_name_from_keyboard() {
    String name = keyboard("", 32, "Enter a name for the iButton:");
    name.trim();
    if (name.length() == 0) { name = "ibutton"; }
    return name;
}

void draw_ibutton_saved() {
    tft.setCursor(32, 57);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.print("iButton saved");
}
void draw_ibutton_save_failed() {
    // it can be saved only if id is correct so we dont need to clear crc error
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.print("iButton save failed");
}
void add_to_ibuttons_array(const String name, const std::array<byte, 8> &id) {
    saved_ibuttons.emplace(saved_ibuttons.begin(), name, id);
}
void append_ibutton_id_to_file(const String name, const String formattedId) {
    File f = LittleFS.open(FILENAME, "a");
    if (f) {
        f.print(name);
        f.print(" ");
        f.print(formattedId);
        f.print("\n");
        add_to_ibuttons_array(name, parse_byte_string(formattedId));
    }

    if (f) {
        draw_ibutton_saved();
    } else {
        draw_ibutton_save_failed();
    }
    f.close();
    delay_with_input_handling(1000);
    draw_read();
}

byte get_crc8(const byte *data, uint8_t length) { return OneWire::crc8(data, length); }
bool check_id_already_saved() {
    for (const auto &ibutton : saved_ibuttons) {
        if (ibutton.second == std::to_array(current_id)) { return true; }
    }
    return false;
}

void draw_read() {
    drawMainBorderWithTitle("Read iButton", true);
    if (has_id) {
        draw_ibutton_id(format_ibutton_id(current_id));
        if (has_error) { draw_ibutton_crc_error(current_crc, current_id[7]); }
    } else {
        draw_no_ibutton();
    }
    draw_button_help();
}
void draw_already_saved() {
    draw_black_over_id();
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.print("iButton already saved");
    delay_with_input_handling(1000);
    draw_black_over_id();
    draw_ibutton_id(format_ibutton_id(current_id));
    if (has_error) { draw_ibutton_crc_error(current_crc, current_id[7]); }
}
void draw_black_over_id() {
    tft.setCursor(12, 57);
    tft.fillRect(12, 57, 220, 30, bruceConfig.bgColor);
}
void draw_no_ibutton_error() {
    draw_black_over_id();
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.print("No iButton detected");
    delay_with_input_handling(1000);
    draw_no_ibutton();
}
void handle_input() {
    if (check(SelPress)) {
        if (!has_id) {
            draw_no_ibutton_error();
        } else {
            if (!check_id_already_saved()) {

                append_ibutton_id_to_file(get_name_from_keyboard(), format_ibutton_id(current_id));
            } else {
                draw_already_saved();
            }
        }
    }

    if (check(EscPress)) { returnToMenu = true; }
}
void delay_with_input_handling(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (check(EscPress)) {
            returnToMenu = true;
            break;
        }
        if (check(SelPress)) {
            return; // we want to break the delay if user presses select, to make the UI more responsive
        }
        delay(50);
    }
}

bool read_rom(byte *buffer) {
    if (one_wire == nullptr) return false;
    if (one_wire->reset()) {
        one_wire->write(0x33);
        one_wire->read_bytes(buffer, 8);
        return true;
    }
    return false;
}

void draw_ibutton_id(String id) {
    draw_black_over_id();
    tft.setCursor(12, 57);
    tft.setTextColor(bruceConfig.priColor);
    tft.setTextSize(1);
    tft.print("ID " + id);
}
void draw_ibutton_crc_error(byte crc, byte expected_crc) {
    draw_black_over_crc_error();
    tft.setTextColor(TFT_RED);
    tft.setTextSize(1);
    tft.print("CRC Error: ");
    tft.print(crc, HEX);
    tft.print(" != ");
    tft.print(expected_crc, HEX);
}

void draw_black_over_crc_error() {
    tft.setCursor(12, 77);
    tft.fillRect(12, 77, 220, 30, bruceConfig.bgColor);
}
void draw_button_help() {
    tft.setTextColor(bruceConfig.priColor);
    tft.setTextSize(1);
    tft.setCursor(12, 117);
    tft.print("Press SEL to save iButton ID");
}
void draw_no_ibutton() {
    draw_black_over_id();
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.print("Waiting for iButton...");
}
} // namespace
void read_ibutton_run() {
    init_onewire();
    parse_saved_ibuttons(saved_ibuttons);
    draw_read();

    while (!returnToMenu) {
        boolean isCaptured = read_rom(id_buffer);
        if (isCaptured) {
            has_id = true;

            memcpy(previous_id, current_id, 8);
            memcpy(current_id, id_buffer, 8);

            byte crc = get_crc8(current_id, 7);
            current_crc = crc;
            has_error = crc != current_id[7];

            if (memcmp(current_id, previous_id, 8)) { draw_ibutton_id(format_ibutton_id(current_id)); }
            if (has_error) {
                crc_changed = (crc != last_crc) || (current_id[7] != last_expected_crc);
                if (crc_changed && has_error) { draw_ibutton_crc_error(crc, current_id[7]); }
            } else if (previous_has_error && !has_error) {
                draw_black_over_crc_error();
            }
            last_crc = crc;
            last_expected_crc = current_id[7];
        }

        previous_has_error = has_error;
        handle_input();
        delay(50); // prevent cpu hogging
    }
    cleanup_onewire();
    cleanup_variables();
}
