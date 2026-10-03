#include "ibutton_lister.h"
#include "core/display.h"
#include "core/sd_functions.h"
#include "ibutton_helper.h"
#include "ibutton_ui.h"
#include <LittleFS.h>
#include <SD.h>
#include <globals.h>

namespace {
enum class KeyAction { NONE, WRITE, RENAME, DELETE, EXPORT };

bool confirm_delete(const String &name) {
    bool confirmed = false;
    String title = "Delete " + name + "?";
    std::vector<Option> choices = {
        {"Cancel", []() {}                             },
        {"Delete", [&confirmed]() { confirmed = true; }},
    };
    loopOptions(choices, MENU_TYPE_SUBMENU, title.c_str());
    return confirmed;
}

void export_key(const SavedIButton &key) {
    FS *fs = nullptr;
    if (!getFsStorage(fs)) {
        displayError("No storage available", true);
        return;
    }
    String path = export_ibtn_file(*fs, key);
    if (path.length() == 0) displayError("Export failed", true);
    else displaySuccess("Exported " + path, true);
}

void manage_key(std::vector<SavedIButton> &saved, int index) {
    while (!returnToMenu) {
        const SavedIButton key = saved[index];
        ib_screen("Saved iButton");
        ib_line(0, key.name, bruceConfig.priColor);
        ib_key_info(1, key.id);
        ib_footer("SEL: options  ESC: back");
        if (!ib_wait_select()) return;

        KeyAction action = KeyAction::NONE;
        std::vector<Option> actions = {
            {"Write to blank key", [&action]() { action = KeyAction::WRITE; } },
            {"Rename",             [&action]() { action = KeyAction::RENAME; }},
            {"Delete",             [&action]() { action = KeyAction::DELETE; }},
            {"Export .ibtn",       [&action]() { action = KeyAction::EXPORT; }},
            {"Back",               []() {}                                    },
        };
        loopOptions(actions, MENU_TYPE_SUBMENU, key.name.c_str());

        switch (action) {
            case KeyAction::WRITE:
                ibutton_write_screen(key.id, key.name);
                ibutton_end();
                break;
            case KeyAction::RENAME: {
                String name;
                if (!ibutton_prompt_name(key.name, key.name, name) || name == key.name) break;
                saved[index].name = name;
                if (!write_saved_ibuttons(saved)) {
                    saved[index].name = key.name;
                    displayError("Rename failed", true);
                }
                break;
            }
            case KeyAction::DELETE:
                if (!confirm_delete(key.name)) break;
                saved.erase(saved.begin() + index);
                if (write_saved_ibuttons(saved)) {
                    displaySuccess("Deleted " + key.name);
                    delay(800);
                    return;
                }
                saved.insert(saved.begin() + index, key);
                displayError("Delete failed", true);
                break;
            case KeyAction::EXPORT: export_key(key); break;
            case KeyAction::NONE: break;
        }
    }
}
} // namespace

void list_ibuttons_run() {
    int last_index = 0;
    while (!returnToMenu) {
        std::vector<SavedIButton> saved = load_saved_ibuttons();
        if (saved.empty()) {
            displayInfo("No saved iButtons yet", true);
            return;
        }

        int picked = -1;
        std::vector<Option> list;
        // Newest first
        for (int i = saved.size() - 1; i >= 0; i--) {
            list.push_back({saved[i].name, [i, &picked]() { picked = i; }});
        }
        if (last_index >= (int)list.size()) last_index = list.size() - 1;
        last_index = loopOptions(list, MENU_TYPE_REGULAR, "Saved iButtons", last_index);
        if (picked < 0) return; // ESC

        manage_key(saved, picked);
    }
}

void import_ibutton_run() {
    FS *fs = &LittleFS;
    if (sdcardMounted) {
        fs = nullptr;
        std::vector<Option> sources = {
            {"SD card",  [&fs]() { fs = &SD; }      },
            {"LittleFS", [&fs]() { fs = &LittleFS; }},
        };
        loopOptions(sources, MENU_TYPE_SUBMENU, "Import from");
        if (fs == nullptr) return;
    }

    String root = fs->exists(IBUTTON_EXPORT_DIR) ? String(IBUTTON_EXPORT_DIR) : String("/");
    String path = loopSD(*fs, true, "ibtn", root);
    if (path.length() == 0) return;

    IButtonId id;
    if (!read_ibtn_file(*fs, path, id)) {
        displayError("No 8-byte iButton ID in file", true);
        return;
    }

    // Suggest the file name (without folder and extension) as the key name
    String name = path.substring(path.lastIndexOf('/') + 1);
    int dot = name.lastIndexOf('.');
    if (dot > 0) name = name.substring(0, dot);
    ibutton_save_key(id, name);
}
