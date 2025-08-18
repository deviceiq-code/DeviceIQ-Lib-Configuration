# DeviceIQ Lib Configuration

`DeviceIQ Lib Configuration` is a lightweight configuration management library designed for embedded systems based on ESP32/ESP8266. 
It allows developers to store, load, and update application settings in JSON format while ensuring safe and reliable persistence using LittleFS.

With atomic file operations, it prevents data corruption during unexpected resets or power loss, and its built-in debounce mechanism reduces flash wear by controlling the frequency of writes.

The library integrates seamlessly with `DevIQ_FileSystem` and leverages ArduinoJson for fast and memory-efficient JSON parsing and serialization.

Library for managing JSON configuration files on ESP32/ESP8266, with persistence in LittleFS and atomic saving.

Features
--------
- Stores configuration in JSON format using ArduinoJson.
- Reads and writes directly to LittleFS using the `DevIQ_FileSystem` library.
- Supports atomic saving (`.tmp` + rename).
- Save control to avoid excessive writes (debounce).
- Immediate (critical) or deferred saving.
- Reset to default configuration from `/def-<filename>`.

Usage Example
-------------

```cpp
#include <DevIQ_FileSystem.h>
#include <DevIQ_Configuration.h>

using namespace DeviceIQ_FileSystem;
using namespace DeviceIQ_Configuration;

Configuration config;

void setup() {
  Serial.begin(115200);

  // Load configuration file
  if (!config.LoadConfigurationFile("/config.json")) {
    Serial.println("Could not load configuration.");
  }

  // Change a value and mark as outdated
  config.Assign(config.Setting["name"], "Device1");

  // Force immediate save
  config.Critical();

  // Or save in a deferred way
  config.Outdated();
}

void loop() {
  // Handles deferred saves
  config.Control();
}
```

API Summary
-----------

### Main Methods

| Method | Description |
|--------|-------------|
| `bool LoadConfigurationFile(String path)` | Loads configuration from the specified file. |
| `bool ResetToDefaultSettings()` | Restores configuration from `/def-<filename>`. |
| `bool SaveSettings()` | Saves immediately. |
| `void Outdated()` | Marks for deferred saving. |
| `bool Critical()` | Saves immediately, ignoring debounce. |
| `void Control()` | Should be called in the loop to perform deferred saves. |
| `void SetMinInterval(uint32_t ms)` | Sets minimum interval between deferred saves. |
| `void SetMaxLatency(uint32_t ms)` | Sets maximum time before saving deferred changes. |
| `template<typename T> bool Assign(JsonVariant dst, const T& value, SaveUrgency u)` | Assigns a value to a field and defines save urgency. |

### Auxiliary Types
- `enum class SaveUrgency { Deferred, Critical }`

Notes
-----
## Dependencies
- [ArduinoJson](https://arduinojson.org/) (v7)
- [LittleFS](https://github.com/lorol/LITTLEFS)
- `DeviceIQ_FileSystem` library

License
-------
This library is released under the MIT License.
