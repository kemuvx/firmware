#include "OneWire.h"
#include "core/display.h"
#include "core/menu_items/iButtonMenu.h"
#include "core/mykeyboard.h"
#include "core/utils.h"

// ========== FORWARD DECLARATIONS ==========
// Drawing functions
void draw_ibutton_saved();
void draw_ibutton_id(String id);
void draw_ibutton_crc_error(byte crc, byte expected_crc);
void draw_black_over_crc_error();
void draw_button_help();
// iButton operations
void init_onewire();
void cleanup_onewire();
void cleanup_variables();
boolean capture_ibutton(byte *buffer);
String format_ibutton_id(const byte *id);
byte get_crc8(const byte *data, uint8_t length);

// File operations
void append_ibutton_id_to_file(const String name, const String formattedId);

// User input
String get_name_from_keyboard();
void handle_input();

// Main function
void read_ibutton_run();
// ==========================================

byte id_buffer[8];
byte current_id[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte previous_id[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool has_id = false;
bool previous_has_error = false;
bool has_error = false;
byte current_crc;
constexpr int IBUTTON_PIN = 0;
OneWire *one_wire = nullptr;

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
    memset(current_id, 0x00, 8);
    memset(previous_id, 0xFF, 8);
    memset(id_buffer, 0, 8);
}
String get_name_from_keyboard() {
    String name = keyboard("", 32, "Enter a name for the iButton:");
    name.replace(" ", "");
    name.replace("-", "");
    return name;
}

String format_ibutton_id(const byte *id) {
    String strId = "";
    for (int i = 0; i < 8; i++) {
        if (id[i] < 16) strId += "0";
        strId += String(id[i], HEX);
        if (i < 7) strId += ":";
    }
    return strId;
}
void draw_ibutton_saved() {
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(2);
    tft.print("iButton saved");
}
void append_ibutton_id_to_file(const String name, const String formattedId) {
    String file_path = "/ibutton_ids.txt";
    String entry = name + "-" + formattedId + "\n";
    LittleFS.open(file_path, "a").print(entry);
    drawMainBorderWithTitle("Read iButton", true);
    draw_ibutton_saved();
    delay(1000);
    drawMainBorderWithTitle("Read iButton", true);
    boolean has_error = current_crc != current_id[7];
    if (has_error) { draw_ibutton_crc_error(current_crc, current_id[7]); }
    draw_ibutton_id(formattedId);
    draw_button_help();
}

byte get_crc8(const byte *data, uint8_t length) { return OneWire::crc8(data, length); }

void handle_input() {
    if (check(SelPress) && has_id) {
        append_ibutton_id_to_file(get_name_from_keyboard(), format_ibutton_id(current_id));
    }

    if (check(EscPress)) { returnToMenu = true; }
}

boolean capture_ibutton(byte *buffer) {
    if (one_wire == nullptr) return false;
    if (one_wire->reset()) {
        one_wire->write(0x33);
        one_wire->read_bytes(buffer, 8);
        return true;
    }
    return false;
}

void draw_ibutton_id(String id) {
    tft.setCursor(12, 57);
    tft.fillRect(12, 57, 220, 30, bruceConfig.bgColor);
    tft.setCursor(12, 57);
    tft.setTextColor(bruceConfig.priColor);
    tft.setTextSize(1);
    tft.print("ID " + id);
}
void draw_ibutton_crc_error(byte crc, byte expected_crc) {
    tft.setCursor(12, 77);
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
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.print("No iButton detected");
}

void read_ibutton_run() {
    init_onewire();

    drawMainBorderWithTitle("Read iButton", true);
    if (!has_id) { draw_no_ibutton(); }
    draw_button_help();

    while (!returnToMenu) {
        boolean isCaptured = capture_ibutton(id_buffer);
        if (isCaptured) {
            has_id = true;

            memcpy(previous_id, current_id, 8);
            memcpy(current_id, id_buffer, 8);

            byte crc = get_crc8(current_id, 7);
            current_crc = crc;
            has_error = crc != current_id[7];

            if (memcmp(current_id, previous_id, 8)) { draw_ibutton_id(format_ibutton_id(current_id)); }

            if (!has_error && previous_has_error) { draw_black_over_crc_error(); }

            if (has_error && !previous_has_error) { draw_ibutton_crc_error(crc, current_id[7]); }
        }
        previous_has_error = has_error;
        handle_input();
        delay(50);
    }
    cleanup_onewire();
    cleanup_variables();
}
