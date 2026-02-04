#ifndef IBUTTON_H
#define IBUTTON_H

#include <OneWire.h>

#ifndef LITE_VERSION
void iButtonStateManager();
bool readOneWire(byte *buffer[8]);
void readingScreen();
void writingScreen();
// void write_ibutton();
// void read_ibutton();
// void write_byte_rw1990(byte data);

#endif

#endif
