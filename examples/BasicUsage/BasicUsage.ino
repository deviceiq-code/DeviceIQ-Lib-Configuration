#include <Arduino.h>
#include <DevIQ_FileSystem.h>
#include <DevIQ_Configuration.h>

using namespace DeviceIQ_FileSystem;
using namespace DeviceIQ_Configuration;

Configuration config;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Starting BasicUsage example...");

  // Attempt to load configuration file
  if (!config.LoadConfigurationFile("/config.json")) {
    Serial.println("Configuration not found. Creating default config...");
    config.Setting = config.JsonConfiguration.to<JsonObject>();
    config.Setting["deviceName"] = "MyDevice";
    config.Setting["version"] = 1;
    config.Critical();
  } else {
    Serial.println("Configuration loaded successfully.");
  }

  // Print current settings
  serializeJsonPretty(config.Setting, Serial);
  Serial.println();

  // Change a value and mark for deferred save
  config.Assign(config.Setting["version"], (int)config.Setting["version"] + 1);

  Serial.println("Version incremented. Will save shortly...");
}

void loop() {
  // Handle deferred saves
  config.Control();
}
