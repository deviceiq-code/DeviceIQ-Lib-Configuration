#ifndef DevIQ_Configuration_h
#define DevIQ_Configuration_h

#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <DevIQ_FileSystem.h>

namespace DeviceIQ_Configuration {
    using namespace DeviceIQ_FileSystem;
    enum class SaveUrgency { Deferred, Critical };

    class Configuration {
        private:
            FileSystem* mFileSystem;
            String mConfigurationFile;
            JsonDocument JsonConfiguration;
            bool _outdated = false;
            uint32_t _firstOutdatedMs = 0;
            uint32_t _lastOutdatedMs = 0;
            uint32_t _minIntervalMs = 500;
            uint32_t _maxLatencyMs = 5000;
            bool SaveSettingsAtomic();
        public:
            inline Configuration() { mFileSystem = new FileSystem(); }
            inline Configuration(FileSystem* fs) { mFileSystem = fs; }
            virtual ~Configuration() {}
            
            bool LoadConfigurationFile(String configurationfile);
            bool ResetToDefaultSettings() { if (mConfigurationFile.isEmpty()) return false; return mFileSystem->CopyFile("/def-" + mConfigurationFile.substring(1), mConfigurationFile); }
            bool SaveSettings() { return SaveSettingsAtomic(); }

            inline void Outdated() { if (!_outdated) _firstOutdatedMs = millis(); _outdated = true; _lastOutdatedMs = millis(); }
            inline bool Critical() { _outdated = false; return SaveSettingsAtomic(); }
            void Control();
            inline void SetMinInterval(uint32_t ms) { _minIntervalMs = ms; }
            inline void SetMaxLatency(uint32_t ms) { _maxLatencyMs = ms; }
            
            template<typename T>
            inline bool Assign(JsonVariant dst, const T& value, SaveUrgency u = SaveUrgency::Deferred) { dst.set(value); if (u == SaveUrgency::Critical) return Critical(); Outdated(); return true; }
            
            JsonObject Setting;
    };
}

#endif