#include <cstdint>
#include <vector>
#include <string>
#include <coreinit/coreinit.h>
#include <vpad/input.h>
#include <wups.h>

struct Color {
    uint8_t r, g, b, a;
};

const Color COLOR_BG           = { 40,  40,  40, 220}; 
const Color COLOR_TAB_ACTIVE   = {100, 220, 100, 255}; 
const Color COLOR_TAB_INACTIVE = { 70,  70,  70, 255}; 
const Color COLOR_BTN_OFF      = {180, 180, 180, 255}; 
const Color COLOR_BTN_ON       = {100, 220, 100, 255}; 

struct HudModule {
    std::string name;
    bool isEnabled;          
    int x, y, width, height; 
};

enum MenuCategory {
    CAT_HUD,
    CAT_ITEM,
    CAT_CAM_MUSIC, 
    CAT_COUNT      
};

enum WiiUButtons {
    VPAD_BUTTON_A      = 0x00000010,
    VPAD_BUTTON_B      = 0x00000020,
    VPAD_BUTTON_LEFT   = 0x00000800,
    VPAD_BUTTON_RIGHT  = 0x00000400,
    VPAD_BUTTON_UP     = 0x00000200,
    VPAD_BUTTON_DOWN   = 0x00000100,
    VPAD_BUTTON_L      = 0x00000040,
    VPAD_BUTTON_ZR     = 0x00000080
};

class MinecraftClient {
private:
    bool isMenuOpen;
    MenuCategory currentCategory; 
    int selectedIndex;            
    bool isFocusingTabs;          

    std::vector<HudModule> hudModules;
    std::vector<HudModule> itemModules;
    std::vector<HudModule> camMusicModules; 

    int targetItemId; 
    float currentZoomLevel; 

    void (*Minecraft_DrawRect)(float x, float y, float w, float h, Color color) = nullptr;
    void (*Minecraft_DrawString)(const char* text, float x, float y, Color color) = nullptr;

    void DrawFilledRect(int x, int y, int w, int h, Color color) {
        if (Minecraft_DrawRect) Minecraft_DrawRect(x, y, w, h, color);
    }

    void DrawText(const std::string& text, int x, int y, Color color) {
        if (Minecraft_DrawString) Minecraft_DrawString(text.c_str(), x, y, color);
    }

public:
    MinecraftClient() {
        isMenuOpen = false;
        currentCategory = CAT_HUD;
        selectedIndex = 0;
        isFocusingTabs = false;
        targetItemId = 264; 
        currentZoomLevel = 1.0f; 

        hudModules.push_back({"armorHUD",   false, 250, 150, 140, 40});
        hudModules.push_back({"keystroke",  false, 410, 150, 140, 40});
        hudModules.push_back({"Nametag",    false, 250, 200, 140, 40});
        hudModules.push_back({"enchantHUD", false, 410, 200, 140, 40});
        hudModules.push_back({"[ Empty ]",  false, 250, 250, 140, 40});

        itemModules.push_back({"Offhand",     false, 250, 150, 140, 40});
        itemModules.push_back({"ItemID Disp", false, 410, 150, 140, 40});
        itemModules.push_back({"Item Craft",  false, 250, 200, 140, 40});

        camMusicModules.push_back({"Music Player", false, 250, 150, 140, 40}); 
        camMusicModules.push_back({"Zoom",         false, 410, 150, 140, 40}); 
        camMusicModules.push_back({"360 Camera",   false, 250, 200, 140, 40}); 
        camMusicModules.push_back({"Shaders",      false, 410, 200, 140, 40}); 
    }

    void ToggleMenu() { isMenuOpen = !isMenuOpen; }
    bool IsOpen() const { return isMenuOpen; }

    std::vector<HudModule>& GetActiveModuleVector() {
        if (currentCategory == CAT_HUD) return hudModules;
        if (currentCategory == CAT_ITEM) return itemModules;
        return camMusicModules;
    }

    void UpdateInput() {
        VPADStatus vpad;
        VPADReadError error;
        
        VPADRead(VPAD_CHAN_0, &vpad, 1, &error);
        if (error != VPAD_READ_SUCCESS) return;

        uint32_t pressed = vpad.trigger; 
        uint32_t held = vpad.hold;       

        if ((held & VPAD_BUTTON_L) && (pressed & VPAD_BUTTON_RIGHT)) {
            ToggleMenu();
            return;
        }

        if (isMenuOpen) {
            std::vector<HudModule>& activeModules = GetActiveModuleVector();

            if (isFocusingTabs) {
                if (pressed & VPAD_BUTTON_DOWN) {
                    currentCategory = static_cast<MenuCategory>((currentCategory + 1) % CAT_COUNT);
                    selectedIndex = 0; 
                }
                else if (pressed & VPAD_BUTTON_UP) {
                    currentCategory = static_cast<MenuCategory>((currentCategory - 1 + CAT_COUNT) % CAT_COUNT);
                    selectedIndex = 0;
                }
                else if (pressed & VPAD_BUTTON_RIGHT) {
                    isFocusingTabs = false; 
                }
            }
            else {
                if (pressed & VPAD_BUTTON_DOWN) {
                    selectedIndex = (selectedIndex + 1) % activeModules.size();
                }
                else if (pressed & VPAD_BUTTON_UP) {
                    selectedIndex = (selectedIndex == 0) ? activeModules.size() - 1 : selectedIndex - 1;
                }
                else if (pressed & VPAD_BUTTON_LEFT) {
                    isFocusingTabs = true; 
                }
                else if (pressed & VPAD_BUTTON_A) {
                    activeModules[selectedIndex].isEnabled = !activeModules[selectedIndex].isEnabled;
                    TriggerFeature(activeModules[selectedIndex].name, activeModules[selectedIndex].isEnabled);
                }
            }
        }
        else {
            bool isZoomEnabled = false;
            for (const auto& mod : camMusicModules) {
                if (mod.name == "Zoom" && mod.isEnabled) {
                    isZoomEnabled = true;
                    break;
                }
            }

            if (isZoomEnabled) {
                if ((held & VPAD_BUTTON_L) && (held & VPAD_BUTTON_ZR)) {
                    if (pressed & VPAD_BUTTON_UP) {
                        currentZoomLevel -= 0.1f;
                        if (currentZoomLevel < 0.2f) currentZoomLevel = 0.2f;
                        ApplyZoomToGame(currentZoomLevel);
                    }
                    else if (pressed & VPAD_BUTTON_DOWN) {
                        currentZoomLevel += 0.1f;
                        if (currentZoomLevel > 1.0f) currentZoomLevel = 1.0f;
                        ApplyZoomToGame(currentZoomLevel);
                    }
                }
            }
        }
    }

    void TriggerFeature(const std::string& name, bool enabled) {
        if (name == "Music Player") {
            if (enabled) InitSdMusicPlayer("sd:/wiiu/AZERACLIENT/MUSIC/");
            else StopSdMusicPlayer();
        }
        else if (name == "Zoom" && !enabled) {
            currentZoomLevel = 1.0f;
            ApplyZoomToGame(currentZoomLevel);
        }
        else if (name == "Item Craft" && enabled) {
            GiveItemToPlayer(targetItemId, 1);
            for(auto& mod : itemModules) { if(mod.name == "Item Craft") mod.isEnabled = false; }
        }
    }

    void ApplyZoomToGame(float fovMultiplier) {
        uint32_t fovAddress = 0x10ABCDEF; 
        if (fovAddress != 0) {
            float baseFov = 70.0f;
            float targetFov = baseFov * fovMultiplier;
            *reinterpret_cast<float*>(fovAddress) = targetFov;
        }
    }

    void InitSdMusicPlayer(const char* path) {}
    void StopSdMusicPlayer() {}
    void GiveItemToPlayer(int id, int count) {}

    void Render() {
        if (!isMenuOpen) return;

        DrawFilledRect(100, 50, 500, 320, COLOR_BG);

        Color hudTabColor = (currentCategory == CAT_HUD) ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE;
        DrawFilledRect(100, 50, 120, 40, hudTabColor);
        DrawText("HUD", 130, 62, {255, 255, 255, 255});

        Color itemTabColor = (currentCategory == CAT_ITEM) ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE;
        DrawFilledRect(100, 95, 120, 40, itemTabColor); 
        DrawText("ITEM", 130, 107, {255, 255, 255, 255});

        Color camMusicTabColor = (currentCategory == CAT_CAM_MUSIC) ? COLOR_TAB_ACTIVE : COLOR_TAB_INACTIVE;
        DrawFilledRect(100, 140, 120, 40, camMusicTabColor); 
        DrawText("CAM&MUSIC", 105, 152, {255, 255, 255, 255}); 

        if (isFocusingTabs) {
            int tabY = (currentCategory == CAT_HUD) ? 50 : (currentCategory == CAT_ITEM) ? 95 : 140;
            DrawFilledRect(96, tabY, 4, 40, {255, 255, 255, 255}); 
        }

        const std::vector<HudModule>& activeModules = GetActiveModuleVector();

        for (size_t i = 0; i < activeModules.size(); ++i) {
            const auto& mod = activeModules[i];
            Color btnColor = mod.isEnabled ? COLOR_BTN_ON : COLOR_BTN_OFF;

            if (!isFocusingTabs && static_cast<int>(i) == selectedIndex) {
                DrawFilledRect(mod.x - 2, mod.y - 2, mod.width + 4, mod.height + 4, {255, 255, 255, 255});
            }

            DrawFilledRect(mod.x, mod.y, mod.width, mod.height, btnColor);
            DrawText(mod.name, mod.x + 10, mod.y + 12, {0, 0, 0, 255});
            
            if (currentCategory == CAT_ITEM && mod.name == "Item Craft") {
                DrawText("ID: " + std::to_string(targetItemId), mod.x + mod.width + 10, mod.y + 12, {255, 255, 255, 255});
            }
            if (currentCategory == CAT_CAM_MUSIC && mod.name == "Zoom" && mod.isEnabled) {
                DrawText("X: " + std::to_string(currentZoomLevel), mod.x + mod.width + 10, mod.y + 12, {255, 255, 255, 255});
            }
        }
    }
};

MinecraftClient g_ClientMenu;

WUPS_PLUGIN_NAME("Azerapvpclient");
WUPS_PLUGIN_VERSION("1.0");
WUPS_PLUGIN_AUTHOR("LUKE");
WUPS_PLUGIN_LICENSE("GPL");

void OnInputAndRenderTick() {
    g_ClientMenu.UpdateInput();
    g_ClientMenu.Render();
}

WUPS_PLUGIN_START() {
    return WUPS_START_OK;
}

WUPS_PLUGIN_END() {
}
