#pragma once
#include "imgui/imgui.h"

// المتغيرات العامة للتحكم في البرنامج
struct Config {
    // Rapid Fire
    bool enableRapidFire = false;
    int rapidFireCPS = 10;
    bool rapidFireActive = false; // حالة التفعيل أثناء الضغط

    // Jitter
    bool enableJitter = false;
    float jitterIntensity = 1.0f;

    // CPS Test
    bool cpsTestRunning = false;
    int cpsClicks = 0;
    float cpsTimer = 0.0f;
    float finalCPS = 0.0f;
    
    // Settings
    bool enableSound = true;
};

extern Config g_Config;

void RenderUI();
void ApplyStyle();
