#ifndef IBUTTON_HELPER_H
#define IBUTTON_HELPER_H

#include <Arduino.h>
#include <FS.h>
#include <array>
#include <vector>

using IButtonId = std::array<byte, 8>;

struct SavedIButton {
    String name;
    IButtonId id;
};

// Saved keys live in LittleFS, one per line: "<name> AA:BB:CC:DD:EE:FF:GG:HH"
constexpr char IBUTTON_FILENAME[] = "/ibuttons.txt";
constexpr char IBUTTON_EXPORT_DIR[] = "/BruceIButton";
constexpr int IBUTTON_NAME_MAX = 32;

// ---------- ID helpers ----------
String format_ibutton_id(const byte *id, char separator = ':');
String format_ibutton_id(const IButtonId &id, char separator = ':');
// Accepts 8 hex bytes separated by ':', '-', ' ' or nothing
bool parse_ibutton_id(String text, IButtonId &out);
byte ibutton_crc(const IButtonId &id);
bool ibutton_crc_valid(const IButtonId &id);
const char *ibutton_family_name(byte family);

// ---------- Saved keys ----------
std::vector<SavedIButton> load_saved_ibuttons();
bool write_saved_ibuttons(const std::vector<SavedIButton> &list);
bool add_saved_ibutton(const String &name, const IButtonId &id);
int find_saved_ibutton(const std::vector<SavedIButton> &list, const IButtonId &id);
String sanitize_ibutton_name(String name);

// ---------- .ibtn files (Bruce/Flipper style) ----------
// Reads the first 8-byte "UID:", "Data:" or "Rom Data:" line of an .ibtn file
bool read_ibtn_file(fs::FS &fs, const String &path, IButtonId &out);
// Writes a Bruce .ibtn file into IBUTTON_EXPORT_DIR, returns the path or "" on failure
String export_ibtn_file(fs::FS &fs, const SavedIButton &key);

// ---------- Hardware ----------
enum class IButtonRead { NO_KEY, OK, CRC_ERROR };
enum class IButtonWrite { OK, NO_KEY, KEY_LOST, VERIFY_FAILED };

int ibutton_pin();
bool ibutton_begin(); // false if no valid pin is configured
void ibutton_end();
bool ibutton_present();
IButtonRead ibutton_read(IButtonId &out);
// Writes an RW1990 blank and reads it back; read_back holds the last ID read from the key
IButtonWrite ibutton_write(const IButtonId &target, IButtonId &read_back);

#endif
