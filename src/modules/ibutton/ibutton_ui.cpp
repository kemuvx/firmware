#include "ibutton_ui.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <driver/gpio.h>
#include <esp_private/esp_gpio_reserve.h>
#include <globals.h>
#include <sdkconfig.h>
#include <soc/spi_pins.h>

namespace {
int text_x() { return BORDER_PAD_X + 2; }
int row_y(int row) { return BORDER_PAD_Y + FM * LH + 6 + row * (FP * LH + 2); }

String fit_line(const String &text) {
    unsigned int max_chars = (tftWidth - 2 * text_x()) / (FP * LW);
    return text.length() > max_chars ? text.substring(0, max_chars) : text;
}

// Pins wired to the flash chip or PSRAM would crash the device if used for 1-Wire
bool pin_reserved(int pin) {
    const int memory_pins[] = {
#ifdef MSPI_IOMUX_PIN_NUM_CLK
        MSPI_IOMUX_PIN_NUM_CLK,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_MISO
        MSPI_IOMUX_PIN_NUM_MISO,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_MOSI
        MSPI_IOMUX_PIN_NUM_MOSI,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_HD
        MSPI_IOMUX_PIN_NUM_HD,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_WP
        MSPI_IOMUX_PIN_NUM_WP,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_CS0
        MSPI_IOMUX_PIN_NUM_CS0,
#endif
#ifdef MSPI_IOMUX_PIN_NUM_CS1
        MSPI_IOMUX_PIN_NUM_CS1,
#endif
#if defined(MSPI_IOMUX_PIN_NUM_D4) && (defined(CONFIG_SPIRAM_MODE_OCT) || defined(CONFIG_ESPTOOLPY_OCT_FLASH))
        MSPI_IOMUX_PIN_NUM_D4,
        MSPI_IOMUX_PIN_NUM_D5,
        MSPI_IOMUX_PIN_NUM_D6,
        MSPI_IOMUX_PIN_NUM_D7,
        MSPI_IOMUX_PIN_NUM_DQS,
#endif
        -1,
    };
    for (int reserved : memory_pins) {
        if (reserved == pin) return true;
    }
    return esp_gpio_is_reserved(BIT64(pin));
}

// Returns false if ESC was pressed while waiting
bool wait_key_removed() {
    while (ibutton_present()) {
        if (check(EscPress) || returnToMenu) return false;
        delay(50);
    }
    return true;
}
} // namespace

// ---------- Layout ----------

void ib_title(const String &title, uint16_t color) {
    drawMainBorder();
    String t = title;
    t.toUpperCase();
    tft.setTextSize(FM);
    tft.setTextColor(color, bruceConfig.bgColor);
    tft.setCursor((tftWidth - (int)t.length() * FM * LW) / 2, BORDER_PAD_Y);
    tft.print(t);
    tft.setTextSize(FP);
}

void ib_screen(const String &title) { ib_title(title, bruceConfig.priColor); }

void ib_clear_line(int row) {
    tft.fillRect(text_x(), row_y(row), tftWidth - 2 * text_x(), FP * LH, bruceConfig.bgColor);
}

void ib_line(int row, const String &text, uint16_t color) {
    ib_clear_line(row);
    tft.setTextSize(FP);
    tft.setTextColor(color, bruceConfig.bgColor);
    tft.setCursor(text_x(), row_y(row));
    tft.print(fit_line(text));
}

void ib_footer(const String &text) {
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    printCenterFootnote(text);
}

void ib_key_info(int first_row, const IButtonId &id) {
    ib_line(first_row, "ID " + format_ibutton_id(id), bruceConfig.priColor);
    ib_line(first_row + 1, String("Type: ") + ibutton_family_name(id[0]), bruceConfig.priColor);
    if (ibutton_crc_valid(id)) {
        ib_line(first_row + 2, "CRC OK", TFT_GREEN);
    } else {
        char text[32];
        snprintf(text, sizeof(text), "CRC error: %02X != %02X", ibutton_crc(id), id[7]);
        ib_line(first_row + 2, text, TFT_RED);
    }
}

bool ib_wait_select() {
    while (true) {
        if (check(EscPress) || returnToMenu) return false;
        if (check(SelPress)) return true;
        delay(20);
    }
}

bool ib_pause(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        if (check(EscPress)) return false;
        if (check(SelPress)) return true;
        delay(20);
    }
    return true;
}

// ---------- Shared flows ----------

bool ibutton_require_pin() {
    if (ibutton_begin()) return true;
    displayWarning("Choose the iButton pin", true);
    ibutton_pin_menu();
    if (ibutton_begin()) return true;
    displayError("No valid iButton pin", true);
    return false;
}

bool ibutton_prompt_name(const String &initial, const String &fallback, String &out) {
    String name = keyboard(initial, IBUTTON_NAME_MAX, "iButton name:");
    if (name == "\x1B") return false; // keyboard cancelled
    name.trim();
    out = sanitize_ibutton_name(name.length() > 0 ? name : fallback);
    return true;
}

bool ibutton_save_key(const IButtonId &id, const String &suggested_name) {
    if (!ibutton_crc_valid(id)) {
        displayError("Bad CRC, not saved", true);
        return false;
    }
    std::vector<SavedIButton> saved = load_saved_ibuttons();
    int existing = find_saved_ibutton(saved, id);
    if (existing >= 0) {
        displayInfo("Already saved as " + saved[existing].name, true);
        return true;
    }

    String name;
    String fallback = "iButton " + String(saved.size() + 1);
    if (!ibutton_prompt_name(suggested_name, fallback, name)) return false;
    if (!add_saved_ibutton(name, id)) {
        displayError("Save failed", true);
        return false;
    }
    displaySuccess("Saved as " + name);
    delay(800);
    return true;
}

void ibutton_write_screen(const IButtonId &target, const String &name) {
    if (!ibutton_require_pin()) return;

    while (true) {
        ib_screen("Write iButton");
        ib_line(0, "Key: " + name, bruceConfig.priColor);
        ib_line(1, "ID " + format_ibutton_id(target), bruceConfig.priColor);
        ib_footer("ESC: back");

        // A key that already holds this ID is usually the original (or the copy just made)
        IButtonId on_probe = {};
        if (ibutton_read(on_probe) != IButtonRead::NO_KEY && on_probe == target) {
            ib_line(3, "Remove the key first", TFT_YELLOW);
            if (!wait_key_removed()) return;
        }
        ib_line(3, "Touch a blank RW1990 key", TFT_YELLOW);

        while (!ibutton_present()) {
            if (check(EscPress) || returnToMenu) return;
            delay(50);
        }
        delay(150); // let the contact settle before writing
        if (!ibutton_present()) continue;

        IButtonId before = {};
        bool had_id = ibutton_read(before) != IButtonRead::NO_KEY;
        bool already_written = had_id && before == target;
        IButtonId after = before;
        IButtonWrite result = IButtonWrite::OK;
        if (!already_written) {
            ib_line(3, "Writing... hold the key", TFT_YELLOW);
            result = ibutton_write(target, after);
        }

        if (result == IButtonWrite::OK) {
            ib_title("Success", TFT_GREEN);
            ib_line(0, "Key: " + name, bruceConfig.priColor);
            ib_line(1, "ID " + format_ibutton_id(target), bruceConfig.priColor);
            if (already_written) {
                ib_line(3, "Key already had this ID", TFT_GREEN);
            } else {
                ib_line(3, "Verified by reading it back", TFT_GREEN);
                if (had_id) ib_line(4, "Was " + format_ibutton_id(before), bruceConfig.priColor);
            }
            ib_footer("SEL: write another  ESC: back");
        } else {
            ib_title("Failed", TFT_RED);
            ib_line(0, "Want " + format_ibutton_id(target), bruceConfig.priColor);
            if (result == IButtonWrite::VERIFY_FAILED) {
                bool got_id = after != IButtonId{};
                ib_line(
                    1, "Got  " + (got_id ? format_ibutton_id(after) : String("nothing")), bruceConfig.priColor
                );
                ib_line(3, "Use a writable RW1990 blank", TFT_YELLOW);
                ib_line(4, "and hold it still", TFT_YELLOW);
            } else {
                ib_line(3, "Key removed while writing", TFT_YELLOW);
                ib_line(4, "Hold it until it finishes", TFT_YELLOW);
            }
            ib_footer("SEL: try again  ESC: back");
        }

        if (!ib_wait_select()) return;
    }
}

void ibutton_pin_menu() {
    int current = ibutton_pin();
    int current_index = 0;
    int selected = -1;
    std::vector<Option> pins;
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pin) || pin_reserved(pin)) continue;
        if (pin == current) current_index = pins.size();
        String label = "GPIO " + String(pin) + (pin == current ? " (current)" : "");
        pins.push_back({label, [pin, &selected]() { selected = pin; }});
    }
    loopOptions(pins, MENU_TYPE_REGULAR, "iButton pin", current_index);
    if (selected < 0 || selected == current) return;

    bruceConfigPins.setiButtonPin(selected);
    ibutton_end();
    displaySuccess("iButton pin: GPIO " + String(selected));
    delay(800);
}
