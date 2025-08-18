#include "DevIQ_Configuration.h"

using namespace DeviceIQ_Configuration;

bool Configuration::LoadConfigurationFile(String configurationfile) {
    if (!mFileSystem->Initialized()) return false;

    mConfigurationFile = configurationfile;

    fs::File f = mFileSystem->OpenFile(mConfigurationFile, "r");
    if (!f) return false;

    DeserializationError err = deserializeJson(JsonConfiguration, f);

    f.close();

    if (err) return false;

    Setting = JsonConfiguration.as<JsonObject>();
    return Setting != nullptr;
}

void Configuration::Control() {
    if (!_outdated) return;
    
    const uint32_t now = millis();
    const bool idleLongEnough = (now - _lastOutdatedMs) >= _minIntervalMs;
    const bool exceededMaxLat = (now - _firstOutdatedMs) >= _maxLatencyMs;
    
    if (idleLongEnough || exceededMaxLat) { if (SaveSettingsAtomic()) _outdated = false; }
}

bool Configuration::SaveSettingsAtomic() {
    if (mConfigurationFile.isEmpty()) return false;
    if (!mFileSystem->Initialized()) return false;

    String tmpPath = mConfigurationFile + ".tmp";
    
    fs::File fTmp = mFileSystem->OpenFile(tmpPath, "w");
    if (!fTmp) return false;

    if (serializeJson(Setting, fTmp) == 0) {
        fTmp.close();
        mFileSystem->RemoveFile(tmpPath);
        return false;
    }

    fTmp.flush();
    fTmp.close();

    if (!mFileSystem->RenameFile(tmpPath, mConfigurationFile)) {
        mFileSystem->RemoveFile(mConfigurationFile);
        if (!mFileSystem->RenameFile(tmpPath, mConfigurationFile)) {
            mFileSystem->RemoveFile(tmpPath);
            return false;
        }
    }
    
    return true;
}