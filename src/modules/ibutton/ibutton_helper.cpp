#include "ibutton_helper.h"
#include "OneWire.h"
#include <FS.h>
#include <LittleFS.h>
#include <driver/gpio.h>
#include <globals.h>

namespace {
constexpr char IBUTTON_TMP_FILENAME[] = "/ibuttons.tmp";
constexpr char IBUTTON_BAK_FILENAME[] = "/ibuttons.bak";

OneWire *bus = nullptr;
int bus_pin = -1;

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool all_bytes_are(const IButtonId &id, byte value) {
    for (byte b : id) {
        if (b != value) return false;
    }
    return true;
}

void remove_if_exists(const char *path) {
    if (LittleFS.exists(path)) LittleFS.remove(path);
}

bool file_ends_without_newline(const char *path) {
    if (!LittleFS.exists(path)) return false;
    File f = LittleFS.open(path, FILE_READ);
    if (!f) return false;
    bool missing = false;
    if (f.size() > 0) {
        f.seek(f.size() - 1);
        missing = f.read() != '\n';
    }
    f.close();
    return missing;
}

String saved_line(const String &name, const IButtonId &id) {
    return sanitize_ibutton_name(name) + " " + format_ibutton_id(id) + "\n";
}

// ---------- RW1990 low level ----------

void line_low_pulse(unsigned int low_us) {
    digitalWrite(bus_pin, LOW);
    pinMode(bus_pin, OUTPUT);
    delayMicroseconds(low_us);
    pinMode(bus_pin, INPUT);
    digitalWrite(bus_pin, HIGH);
}

// RW1990 data bits are sent inverted: a 1 is a long low pulse, a 0 a very short one.
// Each bit needs ~10ms to be programmed.
void rw1990_write_byte(byte data) {
    for (int bit = 0; bit < 8; bit++) {
        digitalWrite(bus_pin, LOW);
        pinMode(bus_pin, OUTPUT);
        if (data & 1) delayMicroseconds(60);
        pinMode(bus_pin, INPUT);
        digitalWrite(bus_pin, HIGH);
        delay(10);
        data >>= 1;
    }
}

// Sequence used by Bruce's original iButton tool
void rw1990_write_bruce(const IButtonId &id) {
    bus->skip();
    bus->reset();
    bus->write(0x33); // Read ROM

    bus->skip();
    bus->reset();
    bus->write(0x3C); // Write mode for some blanks
    delay(50);

    bus->skip();
    bus->reset();
    bus->write(0xD1); // Unlock writing
    delay(50);
    line_low_pulse(60);
    delay(10);

    bus->skip();
    bus->reset();
    bus->write(0xD5); // Write ROM
    delay(50);
    for (byte b : id) {
        rw1990_write_byte(b);
        delayMicroseconds(25);
    }

    bus->reset();
    bus->skip();
    bus->write(0xD1); // Lock writing
    delayMicroseconds(16);
    bus->reset();
}

// Classic RW1990 sequence used by most Arduino duplicators
void rw1990_write_classic(const IButtonId &id) {
    bus->skip();
    bus->reset();
    bus->write(0xD1); // Unlock writing
    line_low_pulse(60);
    delay(10);

    bus->skip();
    bus->reset();
    bus->write(0xD5); // Write ROM
    for (byte b : id) rw1990_write_byte(b);

    bus->reset();
    bus->write(0xD1); // Lock writing
    line_low_pulse(10);
    delay(10);
}

bool read_back_matches(const IButtonId &target, IButtonId &read_back) {
    for (int attempt = 0; attempt < 5; attempt++) {
        delay(20);
        IButtonId id;
        if (ibutton_read(id) == IButtonRead::NO_KEY) continue;
        read_back = id;
        if (id == target) return true;
    }
    return false;
}
} // namespace

// ---------- ID helpers ----------

String format_ibutton_id(const byte *id, char separator) {
    static const char hex[] = "0123456789ABCDEF";
    String out;
    out.reserve(23);
    for (int i = 0; i < 8; i++) {
        if (i > 0 && separator) out += separator;
        out += hex[id[i] >> 4];
        out += hex[id[i] & 0x0F];
    }
    return out;
}

String format_ibutton_id(const IButtonId &id, char separator) {
    return format_ibutton_id(id.data(), separator);
}

bool parse_ibutton_id(String text, IButtonId &out) {
    text.trim();
    IButtonId id = {};
    int count = 0;
    int high = -1;
    for (unsigned int i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c == ':' || c == '-' || c == ' ') {
            if (high >= 0) return false; // separator in the middle of a byte
            continue;
        }
        int value = hex_value(c);
        if (value < 0) return false;
        if (high < 0) {
            high = value;
        } else {
            if (count >= 8) return false;
            id[count++] = (high << 4) | value;
            high = -1;
        }
    }
    if (count != 8 || high >= 0) return false;
    out = id;
    return true;
}

byte ibutton_crc(const IButtonId &id) { return OneWire::crc8(id.data(), 7); }

bool ibutton_crc_valid(const IButtonId &id) { return id[0] != 0x00 && ibutton_crc(id) == id[7]; }

const char *ibutton_family_name(byte family) {
    switch (family) {
        case 0x01: return "DS1990A / DS2401";
        case 0x02: return "DS1991";
        case 0x04: return "DS1994";
        case 0x06: return "DS1993";
        case 0x08: return "DS1992";
        case 0x09: return "DS1982";
        case 0x0A: return "DS1995";
        case 0x0B: return "DS1985";
        case 0x0C: return "DS1996";
        case 0x0F: return "DS1986";
        case 0x10: return "DS18S20 temp sensor";
        case 0x14: return "DS1971";
        case 0x1A: return "DS1963L";
        case 0x21: return "DS1921 Thermochron";
        case 0x22: return "DS1822 temp sensor";
        case 0x23: return "DS1973";
        case 0x28: return "DS18B20 temp sensor";
        case 0x2D: return "DS1972";
        case 0x33: return "DS1961S";
        default: return "Unknown";
    }
}

// ---------- Saved keys ----------

String sanitize_ibutton_name(String name) {
    String clean;
    clean.reserve(name.length());
    for (unsigned int i = 0; i < name.length(); i++) {
        char c = name[i];
        clean += ((uint8_t)c < 0x20 || c == 0x7F) ? ' ' : c;
    }
    clean.trim();
    if (clean.length() > IBUTTON_NAME_MAX) {
        clean = clean.substring(0, IBUTTON_NAME_MAX);
        clean.trim();
    }
    if (clean.length() == 0) clean = "iButton";
    return clean;
}

std::vector<SavedIButton> load_saved_ibuttons() {
    std::vector<SavedIButton> list;
    if (!LittleFS.exists(IBUTTON_FILENAME)) return list;
    File f = LittleFS.open(IBUTTON_FILENAME, FILE_READ);
    if (!f) return list;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line[0] == '#') continue;

        // The ID is the last word, so names may contain spaces
        int split = line.lastIndexOf(' ');
        SavedIButton key;
        if (!parse_ibutton_id(split < 0 ? line : line.substring(split + 1), key.id)) continue;
        key.name = sanitize_ibutton_name(split < 0 ? String("") : line.substring(0, split));
        list.push_back(key);
    }
    f.close();
    return list;
}

bool write_saved_ibuttons(const std::vector<SavedIButton> &list) {
    // Write a temp file first so a failed write never loses the saved keys
    File f = LittleFS.open(IBUTTON_TMP_FILENAME, FILE_WRITE);
    if (!f) return false;
    bool ok = true;
    for (const auto &key : list) {
        String line = saved_line(key.name, key.id);
        if (f.print(line) != line.length()) {
            ok = false;
            break;
        }
    }
    f.close();
    if (!ok) {
        remove_if_exists(IBUTTON_TMP_FILENAME);
        return false;
    }

    remove_if_exists(IBUTTON_BAK_FILENAME);
    bool had_file = LittleFS.exists(IBUTTON_FILENAME);
    if (had_file && !LittleFS.rename(IBUTTON_FILENAME, IBUTTON_BAK_FILENAME)) {
        remove_if_exists(IBUTTON_TMP_FILENAME);
        return false;
    }
    if (!LittleFS.rename(IBUTTON_TMP_FILENAME, IBUTTON_FILENAME)) {
        if (had_file) LittleFS.rename(IBUTTON_BAK_FILENAME, IBUTTON_FILENAME);
        return false;
    }
    remove_if_exists(IBUTTON_BAK_FILENAME);
    return true;
}

bool add_saved_ibutton(const String &name, const IButtonId &id) {
    bool needs_newline = file_ends_without_newline(IBUTTON_FILENAME);
    File f = LittleFS.open(IBUTTON_FILENAME, FILE_APPEND);
    if (!f) return false;
    String line = saved_line(name, id);
    if (needs_newline) line = "\n" + line;
    bool ok = f.print(line) == line.length();
    f.close();
    return ok;
}

int find_saved_ibutton(const std::vector<SavedIButton> &list, const IButtonId &id) {
    for (size_t i = 0; i < list.size(); i++) {
        if (list[i].id == id) return i;
    }
    return -1;
}

// ---------- .ibtn files ----------

bool read_ibtn_file(fs::FS &fs, const String &path, IButtonId &out) {
    File f = fs.open(path, FILE_READ);
    if (!f) return false;
    bool found = false;
    while (f.available() && !found) {
        String line = f.readStringUntil('\n');
        int colon = line.indexOf(':');
        if (colon < 0) continue;
        String field = line.substring(0, colon);
        field.trim();
        if (!field.equalsIgnoreCase("UID") && !field.equalsIgnoreCase("Data") &&
            !field.equalsIgnoreCase("Rom Data"))
            continue;
        found = parse_ibutton_id(line.substring(colon + 1), out);
    }
    f.close();
    return found;
}

String export_ibtn_file(fs::FS &fs, const SavedIButton &key) {
    if (!fs.exists(IBUTTON_EXPORT_DIR) && !fs.mkdir(IBUTTON_EXPORT_DIR)) return "";

    String base;
    for (unsigned int i = 0; i < key.name.length(); i++) {
        char c = key.name[i];
        if (isalnum((unsigned char)c) || c == '-' || c == '_') base += c;
        else if (c == ' ') base += '_';
    }
    if (base.length() == 0) base = format_ibutton_id(key.id, '\0');

    String dir = IBUTTON_EXPORT_DIR;
    String path = dir + "/" + base + ".ibtn";
    for (int n = 1; fs.exists(path); n++) path = dir + "/" + base + "_" + String(n) + ".ibtn";

    File f = fs.open(path, FILE_WRITE);
    if (!f) return "";
    // Same layout as Bruce's Others > iButton tool so either one can load it
    char crc[3];
    snprintf(crc, sizeof(crc), "%02X", key.id[7]);
    f.println("Filetype: Bruce iButton File");
    f.println("Version 1");
    f.println("Device type: DS1990A");
    f.println("UID: " + format_ibutton_id(key.id, ' '));
    f.println(String("CRC: ") + crc);
    f.close();
    return path;
}

// ---------- Hardware ----------

int ibutton_pin() { return bruceConfigPins.iButton; }

bool ibutton_begin() {
    int pin = ibutton_pin();
    if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return false;
    if (bus != nullptr && bus_pin == pin) return true;
    ibutton_end();
    bus = new OneWire(pin);
    bus_pin = pin;
    return true;
}

void ibutton_end() {
    if (bus != nullptr) {
        delete bus;
        bus = nullptr;
    }
    bus_pin = -1;
}

bool ibutton_present() { return bus != nullptr && bus->reset(); }

IButtonRead ibutton_read(IButtonId &out) {
    if (bus == nullptr || !bus->reset()) return IButtonRead::NO_KEY;
    bus->write(0x33); // Read ROM
    IButtonId id;
    bus->read_bytes(id.data(), id.size());
    if (all_bytes_are(id, 0xFF)) return IButtonRead::NO_KEY; // nothing answered
    out = id;
    return ibutton_crc_valid(id) ? IButtonRead::OK : IButtonRead::CRC_ERROR;
}

IButtonWrite ibutton_write(const IButtonId &target, IButtonId &read_back) {
    read_back = {};
    if (!ibutton_present()) return IButtonWrite::NO_KEY;

    // Blanks differ in which sequence they accept, so try both and verify each time
    void (*const sequences[])(const IButtonId &) = {rw1990_write_bruce, rw1990_write_classic};
    for (auto write_sequence : sequences) {
        write_sequence(target);
        if (read_back_matches(target, read_back)) return IButtonWrite::OK;
        if (!ibutton_present()) return IButtonWrite::KEY_LOST;
    }
    return IButtonWrite::VERIFY_FAILED;
}
