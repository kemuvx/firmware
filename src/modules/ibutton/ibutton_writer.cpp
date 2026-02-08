#include "OneWire.h"
#include "core/display.h"
#include "core/menu_items/iButtonMenu.h"
#include "core/utils.h"
#include "ibutton_helper.h"
// ========== FORWARD DECLARATIONS ==========
namespace {
void draw_ibutton_waiting();
void draw_ibutton_help();
void display_ibutton_selection();
void cleanup_variables();
void write_byte(byte data);
std::vector<std::pair<String, std::array<byte, 8>>> saved_ibuttons = {};
enum class state { IBUTTON_SELECTION, IBUTTON_WRITING, IBUTTON_WRITING_PROCESS };
state current_state = state::IBUTTON_SELECTION;
String current_name;
std::array<byte, 8> current_selected_id;
bool screen_changed = true;
byte id_buffer[8];
byte current_id[8];
byte previous_id[8];
byte new_id[8];
byte has_written = false;
bool write_successful = false;
OneWire *one_wire = nullptr;
} // namespace
// ==========================================

namespace {
void cleanup_variables() {
    saved_ibuttons.clear();
    current_state = state::IBUTTON_SELECTION;
    current_selected_id = {};
    screen_changed = true;
    has_written = false;
    write_successful = false;
    memset(id_buffer, 0, 8);
    memset(current_id, 0, 8);
    memset(previous_id, 0, 8);
}
void init_onewire() {
    if (one_wire == nullptr) { one_wire = new OneWire(IBUTTON_PIN); }
}
void cleanup_onewire() {
    if (one_wire != nullptr) {
        delete one_wire;
        one_wire = nullptr;
    }
}
void draw_ibutton_waiting() {
    tft.setCursor(12, 57);
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(1);
    tft.print("Waiting for iButton...");
}

void draw_ibutton_help() {
    tft.setTextColor(bruceConfig.priColor);
    tft.setTextSize(1);
    tft.setCursor(12, 117);
    tft.print("Press NEXT to select another ID");
}
void display_ibutton_selection() {
    screen_changed = false;
    options.clear();
    options.shrink_to_fit();
    for (const auto &[name, id] : saved_ibuttons) {
        options.push_back({name + " " + format_ibutton_id(id.data()), [name, id]() {
                               current_selected_id = id;
                               current_name = name;
                           }});
    }
    addOptionToMainMenu();

    loopOptions(options);
    current_state = state::IBUTTON_WRITING;
    screen_changed = true;
}
void draw_current_ibutton_info() {
    tft.setTextSize(1);
    tft.setTextColor(bruceConfig.priColor);
    tft.setCursor(12, 77);
    tft.print("Current iButton to write:");
    tft.setCursor(12, 87);
    tft.print(current_name);
    tft.print(" ");
    tft.print(format_ibutton_id(current_selected_id.data()));
}
void handle_writing_input() {
    if (check(NextPress)) {
        current_state = state::IBUTTON_SELECTION;
        screen_changed = true;
    }
    if (check(EscPress)) { returnToMenu = true; }
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

void display_ibutton_writing() {
    bool read_successful = read_rom(id_buffer);

    if (screen_changed) {
        drawMainBorderWithTitle("Write iButton", true);
        draw_ibutton_help();
        draw_ibutton_waiting();
        draw_current_ibutton_info();
        screen_changed = false;
    }

    if (read_successful) {
        memcpy(current_id, id_buffer, 8);
        current_state = state::IBUTTON_WRITING_PROCESS;
        screen_changed = true;
    }
    handle_writing_input();
}

void handle_writing_process_input(bool success) {
    if (success) {
        if (check(EscPress)) { returnToMenu = true; }
    } else {
        if (check(EscPress)) { returnToMenu = true; }
        if (check(SelPress)) {
            has_written = false;
            write_successful = false;
            screen_changed = true;
            current_state = state::IBUTTON_WRITING;
            memset(id_buffer, 0, 8);
            memset(current_id, 0, 8);
            memset(previous_id, 0, 8);
        }
    }
}
void ibutton_write_sequence() {
    one_wire->skip();
    one_wire->reset();
    one_wire->write(0xD1);
    digitalWrite(IBUTTON_PIN, LOW);
    pinMode(IBUTTON_PIN, OUTPUT);
    delayMicroseconds(60);
    pinMode(IBUTTON_PIN, INPUT);
    digitalWrite(IBUTTON_PIN, HIGH);
    delay(10);
    one_wire->skip();
    one_wire->reset();
    one_wire->write(0xD5);
    for (byte x = 0; x < 8; x++) { write_byte(current_selected_id.data()[x]); }
    one_wire->write(0xD1);
    digitalWrite(IBUTTON_PIN, LOW);
    pinMode(IBUTTON_PIN, OUTPUT);
    delayMicroseconds(10);
    pinMode(IBUTTON_PIN, INPUT);
    digitalWrite(IBUTTON_PIN, HIGH);
    delay(10);
}

void drawColoredTitle(String title, uint16_t color) {
    tft.setCursor((tftWidth - (title.length() * FM * LW)) / 2, BORDER_PAD_Y);
    tft.setTextColor(color, bruceConfig.bgColor);
    tft.setTextSize(FM);
    title.toUpperCase();
    tft.println(title);
    tft.setTextSize(FP);
}
void display_ibutton_writing_process() {
    if (screen_changed) {
        tft.fillScreen(bruceConfig.bgColor);
        displayTextLine("Writing...");
        screen_changed = false;
    }
    if (!has_written) {
        bool read_successful = read_rom(id_buffer);
        if (read_successful) {
            memcpy(previous_id, current_id, 8);
            ibutton_write_sequence();
            delay(200);
            if (read_rom(id_buffer)) {
                memcpy(current_id, id_buffer, 8);
                if (memcmp(current_id, current_selected_id.data(), 8) == 0) { write_successful = true; }
            }
            delay(50);
        }
        has_written = true;
        tft.fillScreen(bruceConfig.bgColor);
        if (write_successful) {
            drawMainBorder();
            drawColoredTitle("Success", TFT_GREEN);
        } else {
            drawMainBorder();
            drawColoredTitle("Fail", TFT_RED);
        }
        tft.setTextColor(bruceConfig.priColor);
        tft.setTextSize(1);
        tft.setCursor(7, 67);
        tft.print("previous id: ");
        tft.print(format_ibutton_id(previous_id));
        tft.setCursor(7, 77);
        tft.print("current id: ");
        tft.print(format_ibutton_id(current_id));
        tft.setCursor(7, 117);
        if (write_successful) tft.print("Press ESC to exit");
        else tft.print("Press SEL to try again; ESC to exit");
    }

    handle_writing_process_input(write_successful);
}
void write_byte(byte data) {
    int data_bit;
    for (data_bit = 0; data_bit < 8; data_bit++) {
        if (data & 1) {
            digitalWrite(IBUTTON_PIN, LOW);
            pinMode(IBUTTON_PIN, OUTPUT);
            delayMicroseconds(60);
            pinMode(IBUTTON_PIN, INPUT);
            digitalWrite(IBUTTON_PIN, HIGH);
            delay(10);
        } else {
            digitalWrite(IBUTTON_PIN, LOW);
            pinMode(IBUTTON_PIN, OUTPUT);
            pinMode(IBUTTON_PIN, INPUT);
            digitalWrite(IBUTTON_PIN, HIGH);
            delay(10);
        }
        data = data >> 1;
    }
}
} // namespace
void write_ibutton_run() {
    parse_saved_ibuttons(saved_ibuttons);
    displayTextLine("Select ID");
    init_onewire();
    delay(250);

    while (!returnToMenu) {
        switch (current_state) {
            case state::IBUTTON_SELECTION: display_ibutton_selection(); break;
            case state::IBUTTON_WRITING: display_ibutton_writing(); break;
            case state::IBUTTON_WRITING_PROCESS: display_ibutton_writing_process(); break;
        }
        delay(50);
    }
    cleanup_onewire();
    cleanup_variables();
}
