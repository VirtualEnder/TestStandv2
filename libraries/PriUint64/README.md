# PriUint64.h for Arduino

This library allows printing `uint64_t` in Arduino.

```
#include <PriUint64.h>

void setup() {
  Serial.begin();
  Serial.println();

  uint64_t x = 0x9aa567b200;
  Serial.println(PriUint64<HEX>(x));
}

void loop() {
}
```

## Streaming 5 Integration

This library can integrate with [Streaming 5](http://arduiniana.org/libraries/streaming/).
To enable this integration, `#include <Streaming.h>` before including this library.

```
#include <Streaming.h>
#include <PriUint64.h>

void setup() {
  Serial.begin();
  Serial.println();

  uint64_t x = 0x9aa567b200;
  Serial << x << endl; // available on all platforms
  Serial << _HEX(x) << endl; // macros available on ESP8266 and ESP32 only
}

void loop() {
}
```
