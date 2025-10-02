#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <functional>
#include <string>

struct SimulationSettings {
    float m_yaw = 0.022f;
    float cl_yawspeed = 210.0f;
    int leftKey = 0x06; // Mouse4
    int rightKey = 0x05; // Mouse5
    float updateRate = 1000.0f;
    bool autoActivate = true;
    float modifier = 0.5f;
    int modifierKey = 0x12; // VK_MENU (Alt)
    int windowX = -1; // -1 means unset (center window)
    int windowY = -1;
    int autoActivateToggleKey = 0x74; // F5 by default
};

class SettingsManager {
public:
    explicit SettingsManager(const std::string& configFile = "settings.json");
    void load();
    void save() const;
    SimulationSettings get() const;
    void update(std::function<void(SimulationSettings&)> updater);

private:
    std::string configFile_;
    SimulationSettings settings_;
    mutable std::mutex mutex_;
};
