#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#ifndef NATIVE_TEST
#include <Arduino.h>
#else
#include "Arduino.h"
#endif

class SdCardManager {
public:
    static bool begin();
    static bool isMounted();
    static void end();
    static bool isCardPresent();
};

#endif // SD_CARD_MANAGER_H
