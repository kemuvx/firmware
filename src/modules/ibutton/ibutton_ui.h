#ifndef IBUTTON_UI_H
#define IBUTTON_UI_H

#include "ibutton_helper.h"

// ---------- Layout (rows sit between the title and the footer) ----------
void ib_title(const String &title, uint16_t color);
void ib_screen(const String &title);
void ib_line(int row, const String &text, uint16_t color);
void ib_clear_line(int row);
void ib_footer(const String &text);
void ib_key_info(int first_row, const IButtonId &id); // uses 3 rows: ID, type, CRC
bool ib_wait_select();                                // true on SEL, false on ESC
bool ib_pause(unsigned long ms);                      // false if ESC was pressed

// ---------- Shared flows ----------
bool ibutton_require_pin();
bool ibutton_prompt_name(const String &initial, const String &fallback, String &out);
bool ibutton_save_key(const IButtonId &id, const String &suggested_name);
void ibutton_write_screen(const IButtonId &target, const String &name);
void ibutton_pin_menu();

#endif
