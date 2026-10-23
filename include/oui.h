#ifndef OUI_H
#define OUI_H

#include <stdint.h>
#include <stddef.h>

#ifdef NATIVE_TEST
#include "Arduino.h"
#else
#include <Arduino.h>
#endif

#ifndef PROGMEM
#define PROGMEM
#endif

struct OuiEntry {
    uint32_t prefix;
    const char* vendor;
};

// Returns vendor name (e.g., "Apple", "Ubiquiti", etc.) or "Unknown Vendor" if not found
const char* getOuiVendor(uint32_t prefix);
const char* getOuiVendor(const uint8_t* bssid);
const char* getOuiVendor(const char* bssidStr);
const char* getOuiVendor(const String& bssidStr);

// Parse MAC address string (e.g., "AA:BB:CC:DD:EE:FF" or "aabbcc") into 24-bit OUI prefix
bool parseMacPrefix(const char* macStr, uint32_t* prefixOut);

// Table introspection for tests and stats
size_t getOuiTableSize();
const OuiEntry* getOuiTable();

#endif // OUI_H
