#include "iButtonMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/ibutton/ibutton_helper.h"
#include "modules/ibutton/ibutton_lister.h"
#include "modules/ibutton/ibutton_reader.h"
#include "modules/ibutton/ibutton_ui.h"
#include "modules/ibutton/ibutton_writer.h"

void IButtonMenu::optionsMenu() {
    int index = 0;
    // Stay in the iButton menu after each action until ESC or "Main Menu"
    while (true) {
        options = {
            {"Read iButton",                       read_ibutton_run  },
            {"Write iButton",                      write_ibutton_run },
            {"Saved iButtons",                     list_ibuttons_run },
            {"Import .ibtn",                       import_ibutton_run},
            {"Pin: GPIO " + String(ibutton_pin()), ibutton_pin_menu  },
        };
        addOptionToMainMenu();
        index = loopOptions(options, MENU_TYPE_SUBMENU, "iButton", index);
        if (index < 0 || returnToMenu) break;
    }
}

void IButtonMenu::drawIcon(float scale) {
    clearIconArea();

    int radius = scale * 36;
    if (radius % 2 != 0) radius++;

    tft.fillCircle(iconCenterX, iconCenterY, radius, bruceConfig.priColor);
    tft.fillCircle(iconCenterX, iconCenterY, radius - 2, bruceConfig.bgColor);

    tft.fillCircle(iconCenterX, iconCenterY, radius / 1.2, bruceConfig.priColor);
}
