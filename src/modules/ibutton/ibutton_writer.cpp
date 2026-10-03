#include "ibutton_writer.h"
#include "core/display.h"
#include "ibutton_helper.h"
#include "ibutton_ui.h"
#include <globals.h>

void write_ibutton_run() {
    std::vector<SavedIButton> saved = load_saved_ibuttons();
    if (saved.empty()) {
        displayInfo("No saved iButtons yet", true);
        return;
    }

    int last_index = 0;
    while (!returnToMenu) {
        int picked = -1;
        std::vector<Option> list;
        // Newest first
        for (int i = saved.size() - 1; i >= 0; i--) {
            list.push_back({saved[i].name, [i, &picked]() { picked = i; }});
        }
        last_index = loopOptions(list, MENU_TYPE_REGULAR, "Write iButton", last_index);
        if (picked < 0) break; // ESC

        ibutton_write_screen(saved[picked].id, saved[picked].name);
    }
    ibutton_end();
}
