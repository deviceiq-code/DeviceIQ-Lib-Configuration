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

            static inline String trim(const String& s) { int i = 0, j = s.length() - 1; while (i <= j && isspace((unsigned char)s[i])) i++; while (j >= i && isspace((unsigned char)s[j])) j--; return (i > j) ? String() : s.substring(i, j + 1); }
            JsonVariantConst findConst(const char* path) const { if (!path || !*path) return Setting; JsonVariantConst cur = Setting; String token; const char* p = path; while (cur && *p) { const char* sep = strchr(p, '|'); token = trim(sep ? String(p, sep - p) : String(p)); if (token.isEmpty()) return JsonVariantConst(); cur = cur[token]; if (sep) p = sep + 1; else break; } return cur; }
            JsonVariant ensure(const char* path) { if (!path || !*path) return Setting; JsonVariant cur = Setting; const char* p = path; while (cur && *p) { const char* sep = strchr(p, '|'); String key = trim(sep ? String(p, sep - p) : String(p)); if (key.isEmpty()) return JsonVariant(); JsonObject obj = cur.to<JsonObject>(); JsonVariant next = obj[key]; if (next.isNull() || !next.is<JsonObject>()) { obj[key].to<JsonObject>(); next = obj[key]; } cur = next; if (sep) p = sep + 1; else break; } return cur; }

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
            
            template<typename T> inline bool Assign(JsonVariant dst, const T& value, SaveUrgency u = SaveUrgency::Deferred) { dst.set(value); if (u == SaveUrgency::Critical) return Critical(); Outdated(); return true; }
            
            JsonObject Setting;

            template<typename T> T Get(const char* path, T defaultValue = T{}) const { JsonVariantConst v = findConst(path); if (!v.isNull() && v.is<T>()) return v.as<T>(); return defaultValue; }
            const char* Get(const char* path, const char* defaultValue = "") const { JsonVariantConst v = findConst(path); if (!v.isNull() && v.is<const char*>()) return v.as<const char*>(); return defaultValue; }
            template<typename T>
            bool Set(const char* path, const T& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value, u); }
            bool Set(const char* path, const String& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value.c_str(), u); }
            bool Set(const char* path, const char* value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value, u); }
    };
}

#endif