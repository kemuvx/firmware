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
std::vector<std::pair<String, std::array<byte, 8>>> saved_ibuttons = {};
enum class state { IBUTTON_SELECTION, IBUTTON_WRITING };
state current_state = state::IBUTTON_SELECTION;
String current_name;
std::array<byte, 8> current_id;
bool screen_changed = true;
} // namespace
// ==========================================

namespace {
void cleanup_variables() {
    saved_ibuttons.clear();
    current_state = state::IBUTTON_SELECTION;
    current_id = {};
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
    tft.print("Press SEL to select another ID");
}
void display_ibutton_selection() {
    screen_changed = false;
    options.clear();
    options.shrink_to_fit();
    for (const auto &[name, id] : saved_ibuttons) {
        options.push_back({name + " " + format_ibutton_id(id.data()), [name, id]() {
                               current_id = id;
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
    tft.print(format_ibutton_id(current_id.data()));
}
void handle_writing_input() {
    if (check(SelPress)) {
        current_state = state::IBUTTON_SELECTION;
        screen_changed = true;
    }
    if (check(EscPress)) { returnToMenu = true; }
}
void display_ibutton_writing() {
    if (screen_changed) {
        drawMainBorderWithTitle("Write iButton", true);
        draw_ibutton_help();
        draw_ibutton_waiting();
        draw_current_ibutton_info();
        screen_changed = false;
    }
    handle_writing_input();
}
} // namespace

void write_ibutton_run() {
    parse_saved_ibuttons(saved_ibuttons);

    while (!returnToMenu) {
        switch (current_state) {
            case state::IBUTTON_SELECTION: display_ibutton_selection(); break;
            case state::IBUTTON_WRITING: display_ibutton_writing(); break;
        }
        delay(50);
    }
    cleanup_variables();
}
