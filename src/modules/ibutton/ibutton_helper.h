#include <LittleFS.h>
#include <array>
#include <utility>
#include <vector>

constexpr int IBUTTON_PIN = 0;
constexpr char FILENAME[] = "/ibuttons.txt";

std::array<byte, 8> parse_byte_string(String str);

void parse_saved_ibuttons(std::vector<std::pair<String, std::array<byte, 8>>> &saved_ibuttons);

String format_ibutton_id(const byte *id);
