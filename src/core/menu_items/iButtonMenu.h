#ifndef __IBUTTON_MENU_H__
#define __IBUTTON_MENU_H__

#include "MenuItemInterface.h"

class IButtonMenu : public MenuItemInterface {
public:
    IButtonMenu() : MenuItemInterface("iButton") {} // Add constructor

    void optionsMenu(void) override;
    void drawIcon(float scale) override; // Change from draw() to drawIcon()

    // Add these if MenuItemInterface requires them
    bool hasTheme() override {
        // Check if your config has ibutton theme support
        return false; // or bruceConfig.theme.ibutton if it exists
    }

    String themePath() override { return ""; }

private:
    void configMenu(void);
};

#endif
