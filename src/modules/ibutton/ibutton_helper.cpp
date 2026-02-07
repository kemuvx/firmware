#include "ibutton_helper.h"
#include "OneWire.h"
#include "core/display.h"
#include "core/menu_items/iButtonMenu.h"
#include "core/mykeyboard.h"
#include "core/utils.h"
#include <iostream>
#include <utility>
#include <vector>

std::array<byte, 8> parse_byte_string(String str) {
    std::array<byte, 8> arr = {};
    int i = 0;
    int byte_count = 0;
    while (byte_count < 8) {
        String str_byte = str.substring(i, i + 2);
        arr[byte_count] = static_cast<byte>(strtol(str_byte.c_str(), nullptr, 16));
        i += 3;
        byte_count++;
    }
    return arr;
}

void parse_saved_ibuttons(std::vector<std::pair<String, std::array<byte, 8>>> &saved_ibuttons) {
    File f = LittleFS.open(FILENAME, "r");
    if (!f) return;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        int last_space = line.lastIndexOf(' ');
        if (last_space == -1) continue;
        String name = line.substring(0, last_space);
        String byte_string = line.substring(last_space + 1);
        auto byte_arr = parse_byte_string(byte_string);
        saved_ibuttons.emplace(saved_ibuttons.begin(), name, byte_arr);
    }
}

String format_ibutton_id(const byte *id) {
    String strId = "";
    for (int i = 0; i < 8; i++) {
        if (id[i] < 16) strId += "0";
        String hexByte = String(id[i], HEX);
        hexByte.toUpperCase();
        strId += hexByte;
        if (i < 7) strId += ":";
    }
    return strId;
}
