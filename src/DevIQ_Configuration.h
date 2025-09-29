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
            JsonVariantConst findConstAt(const char* arrayPath, size_t index, const char* subPath = nullptr) const { JsonVariantConst base = findConst(arrayPath); if (base.is<JsonArray>()) { JsonArrayConst arr = base.as<JsonArrayConst>(); if (index >= arr.size()) return JsonVariantConst(); JsonVariantConst elem = arr[index]; if (!subPath || !*subPath) return elem; JsonVariantConst cur = elem; String token; const char* p = subPath; while (cur && *p) { const char* sep = strchr(p, '|'); token = String(sep ? String(p, sep - p) : String(p)); cur = cur[token]; if (sep) p = sep + 1; else break; } return cur; } return JsonVariantConst(); }
            JsonVariant ensure(const char* path) { if (!path || !*path) return Setting; JsonVariant cur = Setting; const char* p = path; while (cur && *p) { const char* sep = strchr(p, '|'); String key = trim(sep ? String(p, sep - p) : String(p)); if (key.isEmpty()) return JsonVariant(); JsonObject obj = cur.to<JsonObject>(); JsonVariant next = obj[key]; if (next.isNull() || !next.is<JsonObject>()) { obj[key].to<JsonObject>(); next = obj[key]; } cur = next; if (sep) p = sep + 1; else break; } return cur; }
            JsonVariant ensureFrom(JsonVariant base, const char* subPath) { if (!base || !subPath || !*subPath) return base; auto isNumber = [](const String& s)->bool { if (s.isEmpty()) return false; for (size_t i = 0; i < s.length(); ++i) { if (!isdigit((unsigned char)s[i])) return false; } return true; }; JsonVariant cur = base; String token; const char* p = subPath; while (*p) { const char* sep = strchr(p, '|'); token = trim(sep ? String(p, sep - p) : String(p)); if (token.isEmpty()) return JsonVariant(); if (cur.is<JsonObject>()) { JsonObject obj = cur.as<JsonObject>(); JsonVariant next = obj[token]; if (next.isNull()) { JsonObject child = obj.createNestedObject(token); next = child; } cur = next; } else if (cur.is<JsonArray>()) { if (!isNumber(token)) return JsonVariant(); JsonArray arr = cur.as<JsonArray>(); size_t idx = (size_t)token.toInt(); while (arr.size() <= idx) { arr.add(JsonObject()); } cur = arr[idx]; } else { cur.set(JsonObject()); JsonObject obj = cur.as<JsonObject>(); JsonObject child = obj.createNestedObject(token); cur = child; } if (!sep) break; p = sep + 1; } return cur; }
            JsonVariant ensureArrayElem(const char* arrayPath, size_t index) { JsonVariant arrVar = ensureArray(arrayPath); if (arrVar.isNull()) return JsonVariant(); JsonArray arr = arrVar.as<JsonArray>(); while (arr.size() <= index) { arr.add(JsonObject()); } return arr[index]; }
            JsonVariant ensureArray(const char* arrayPath) { if (!arrayPath || !*arrayPath) return JsonVariant(); JsonVariant cur = Setting; String token, last; const char* p = arrayPath;  const char* sep = nullptr; const char* lastSep = nullptr; while ((sep = strchr(p, '|'))) { token = trim(String(p, sep - p)); if (token.isEmpty()) return JsonVariant(); JsonVariant next = cur[token]; if (next.isNull()) { cur.createNestedObject(token); next = cur[token]; } cur = next; p = sep + 1; lastSep = sep; } last = trim(String(p)); if (last.isEmpty()) return JsonVariant(); JsonVariant arrVar = cur[last]; if (arrVar.isNull()) { cur.createNestedArray(last); arrVar = cur[last]; } else if (!arrVar.is<JsonArray>()) { return JsonVariant(); } return arrVar; }

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
            template<typename T> T GetAt(const char* arrayPath, size_t index, const char* subPath, T def = T{}) const { JsonVariantConst v = findConstAt(arrayPath, index, subPath); return (!v.isNull() && v.is<T>()) ? v.as<T>() : def; }
            const char* GetAt(const char* arrayPath, size_t index, const char* subPath, const char* def = "") const { JsonVariantConst v = findConstAt(arrayPath, index, subPath); return (!v.isNull() && v.is<const char*>()) ? v.as<const char*>() : def; }
            JsonObjectConst GetAt(const char* arrayPath, size_t index, const char* subPath = nullptr) const { JsonVariantConst v = findConstAt(arrayPath, index, subPath); return v.is<JsonObject>() ? v.as<JsonObjectConst>() : JsonObjectConst(); }
            JsonArrayConst GetAtArray(const char* arrayPath, size_t index, const char* subPath = nullptr) const { JsonVariantConst v = findConstAt(arrayPath, index, subPath); return v.is<JsonArray>() ? v.as<JsonArrayConst>() : JsonArrayConst(); }

            template<typename T> bool Set(const char* path, const T& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value, u); }
            bool Set(const char* path, const String& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value.c_str(), u); }
            bool Set(const char* path, const char* value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensure(path); if (dst.isNull()) return false; return Assign(dst, value, u); }
            template<typename T> bool SetAt(const char* arrayPath, size_t index, const T& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant dst = ensureArrayElem(arrayPath, index); if (dst.isNull()) return false; return Assign(dst, value, u); }
            template<typename T> bool SetAt(const char* arrayPath, size_t index, const char* subPath, const T& value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant elem = ensureArrayElem(arrayPath, index); if (elem.isNull()) return false; JsonVariant dst = ensureFrom(elem, subPath); if (dst.isNull()) return false; return Assign(dst, value, u); }
            bool SetAt(const char* arrayPath, size_t index, const char* subPath, const char* value, SaveUrgency u = SaveUrgency::Deferred) { JsonVariant elem = ensureArrayElem(arrayPath, index); if (elem.isNull()) return false; JsonVariant dst = ensureFrom(elem, subPath); if (dst.isNull()) return false; return Assign(dst, value, u); }
            
            uint16_t Elements(const char* path) const { JsonVariantConst v = findConst(path); if (v.is<JsonArray>()) { return v.as<JsonArrayConst>().size(); } if (v.is<JsonObject>()) { return v.as<JsonObjectConst>().size(); } return 0; }
    };
}

#endif