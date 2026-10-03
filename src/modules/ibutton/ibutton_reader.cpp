#include "ibutton_reader.h"
#include "core/display.h"
#include "ibutton_helper.h"
#include "ibutton_ui.h"
#include <globals.h>

namespace {
constexpr int ROW_KEY = 0;    // ID, type and CRC use rows 0-2
constexpr int ROW_SAVED = 3;  // "Saved as ..." or a hint
constexpr int ROW_STATUS = 4; // short-lived messages

struct ReaderState {
    std::vector<SavedIButton> saved;
    IButtonId current = {};
    bool has_key = false;        // something was read since the screen opened
    bool key_on_contact = false; // the probe answered on the last poll
};

void draw_key(const ReaderState &s) {
    if (!s.has_key) {
        ib_line(ROW_KEY, "Waiting for iButton...", TFT_YELLOW);
        for (int row = ROW_KEY + 1; row <= ROW_SAVED; row++) ib_clear_line(row);
        return;
    }
    ib_key_info(ROW_KEY, s.current);
    int saved_index = find_saved_ibutton(s.saved, s.current);
    if (!ibutton_crc_valid(s.current)) ib_line(ROW_SAVED, "Hold the key still", TFT_YELLOW);
    else if (saved_index >= 0) ib_line(ROW_SAVED, "Saved as " + s.saved[saved_index].name, TFT_CYAN);
    else ib_clear_line(ROW_SAVED);
}

void draw_screen(const ReaderState &s) {
    ib_screen("Read iButton");
    draw_key(s);
    ib_footer("SEL: options  ESC: back");
}

void flash_status(const String &text, uint16_t color) {
    ib_line(ROW_STATUS, text, color);
    ib_pause(1200);
    ib_clear_line(ROW_STATUS);
}

// Returns true if the screen needs a full redraw
bool handle_select(ReaderState &s) {
    if (!s.has_key) {
        flash_status("No iButton read yet", TFT_RED);
        return false;
    }
    if (!ibutton_crc_valid(s.current)) {
        flash_status("Bad read, hold the key still", TFT_RED);
        return false;
    }

    int saved_index = find_saved_ibutton(s.saved, s.current);
    int action = 0;
    std::vector<Option> actions;
    if (saved_index < 0) actions.push_back({"Save", [&action]() { action = 1; }});
    actions.push_back({"Write to blank key", [&action]() { action = 2; }});
    actions.push_back({"Back", []() {}});
    loopOptions(actions, MENU_TYPE_SUBMENU, "iButton");

    if (action == 1) {
        ibutton_save_key(s.current, "");
        s.saved = load_saved_ibuttons();
    } else if (action == 2) {
        String name = saved_index >= 0 ? s.saved[saved_index].name : String("Just read");
        ibutton_write_screen(s.current, name);
    }
    return true;
}
} // namespace

void read_ibutton_run() {
    if (!ibutton_require_pin()) return;

    ReaderState s;
    s.saved = load_saved_ibuttons();
    draw_screen(s);

    while (!returnToMenu) {
        IButtonId id;
        IButtonRead result = ibutton_read(id);
        if (result == IButtonRead::NO_KEY) {
            s.key_on_contact = false;
        } else {
            // A glitchy read while a good key stays on the probe is ignored, so the screen doesn't flicker
            bool showing_good = s.has_key && ibutton_crc_valid(s.current);
            bool glitch = result == IButtonRead::CRC_ERROR && showing_good && s.key_on_contact;
            s.key_on_contact = true;
            if (!glitch && (!s.has_key || id != s.current)) {
                s.current = id;
                s.has_key = true;
                draw_key(s);
            }
        }

        if (check(EscPress)) break;
        if (check(SelPress) && handle_select(s)) draw_screen(s);
        delay(50);
    }
    ibutton_end();
}
