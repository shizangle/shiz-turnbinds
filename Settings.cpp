#include "Settings.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

SettingsManager::SettingsManager(const std::string& configFile) : configFile_(configFile) {
    load();
}

void SettingsManager::load() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream file(configFile_);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << configFile_ << ", using default settings\n";
        return;
    }
    try {
        nlohmann::json j;
        file >> j;
        settings_.m_yaw = j.value("m_yaw", settings_.m_yaw);
        settings_.cl_yawspeed = j.value("cl_yawspeed", settings_.cl_yawspeed);
        settings_.leftKey = j.value("leftKey", settings_.leftKey);
        settings_.rightKey = j.value("rightKey", settings_.rightKey);
        settings_.updateRate = j.value("updateRate", settings_.updateRate);
        settings_.autoActivate = j.value("autoActivate", settings_.autoActivate);
        settings_.modifier = j.value("modifier", settings_.modifier);
        settings_.modifierKey = j.value("modifierKey", settings_.modifierKey);
        settings_.windowX = j.value("windowX", settings_.windowX);
        settings_.windowY = j.value("windowY", settings_.windowY);
        settings_.autoActivateToggleKey = j.value("autoActivateToggleKey", settings_.autoActivateToggleKey);
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse " << configFile_ << ": " << e.what() << "\n";
    }
    file.close();
}

void SettingsManager::save() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    j["m_yaw"] = settings_.m_yaw;
    j["cl_yawspeed"] = settings_.cl_yawspeed;
    j["leftKey"] = settings_.leftKey;
    j["rightKey"] = settings_.rightKey;
    j["updateRate"] = settings_.updateRate;
    j["autoActivate"] = settings_.autoActivate;
    j["modifier"] = settings_.modifier;
    j["modifierKey"] = settings_.modifierKey;
    j["windowX"] = settings_.windowX;
    j["windowY"] = settings_.windowY;
    j["autoActivateToggleKey"] = settings_.autoActivateToggleKey;

    std::ofstream file(configFile_);
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
    else {
        std::cerr << "Failed to save settings to " << configFile_ << "\n";
    }
}

SimulationSettings SettingsManager::get() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return settings_;
}

void SettingsManager::update(std::function<void(SimulationSettings&)> updater) {
    std::lock_guard<std::mutex> lock(mutex_);
    updater(settings_);
}
