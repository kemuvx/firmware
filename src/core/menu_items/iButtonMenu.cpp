#include "iButtonMenu.h"
#include "core/display.h"
#include "core/utils.h"
#include "modules/ibutton/ibutton_lister.h"
#include "modules/ibutton/ibutton_reader.h"
#include "modules/ibutton/ibutton_writer.h"

const auto &priColor = bruceConfig.priColor;

void IButtonMenu::optionsMenu() {
    options = {
        {"Read iButton",  [=]() { read_ibutton_run(); } },
        {"Write iButton", [=]() { write_ibutton_run(); }},
        {"List iButtons", [=]() { list_ibuttons_run(); }}
    };
    addOptionToMainMenu();
    loopOptions(options, MENU_TYPE_SUBMENU, "iButton");
}

void IButtonMenu::drawIcon(float scale) {
    clearIconArea();

    int radius = scale * 36;
    if (radius % 2 != 0) radius++;

    tft.fillCircle(iconCenterX, iconCenterY, radius, priColor);
    tft.fillCircle(iconCenterX, iconCenterY, radius - 2, bruceConfig.bgColor);

    tft.fillCircle(iconCenterX, iconCenterY, radius / 1.2, priColor);
}
