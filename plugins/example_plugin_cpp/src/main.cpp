#include <gctypes.h>

#define BUTTON_A        0x0001
#define BUTTON_B        0x0002
#define BUTTON_L        0x0020
#define BUTTON_ZR       0x0080
#define BUTTON_Y        0x0800
#define BUTTON_UP       0x0100
#define BUTTON_DOWN     0x0200

#define MINECRAFT_PLAYER_BASE   0x10A94F00
#define MINECRAFT_FOV_ADDRESS   0x10E45C20

struct ClientSettings {
    bool armorHUD       = false;
    bool nametag        = false;
    bool tntTimer       = false;
    bool keystroke      = false;
    bool hitbox         = false;

    bool antiExplosion  = false;
    bool fireCharge     = false;
    bool offhand        = false;

    bool roadMusic      = false;
    bool zoom           = false;
    bool freelook360    = false;
    bool recording      = false;
    bool screenshot     = false;

    bool villagerEditor = false;
    bool manaita        = false;
};

struct MenuState {
    bool isOpen         = false;
    int currentTab      = 0; 
    int selectedButton  = 0;
};

class AzeraClient {
private:
    ClientSettings settings;
    MenuState menu;
    
    template <typename T>
    void WriteMemory(uint32_t address, T value) {
        T* ptr = (T*)address;
        if (ptr != nullptr) {
            *ptr = value;
        }
    }

public:
    AzeraClient() {
        settings.armorHUD = true; 
    }

    void DrawMenu() {
        if (!menu.isOpen) return;
    }

    void OnFrameUpdate(uint32_t pressed, uint32_t held) {
        if ((pressed & BUTTON_L) && (pressed & BUTTON_B)) {
            menu.isOpen = !menu.isOpen;
        }

        if (menu.isOpen) {
            if (pressed & BUTTON_DOWN)  menu.selectedButton++;
            if (pressed & BUTTON_UP)    menu.selectedButton--;
            if (pressed & BUTTON_A) {
                settings.zoom = !settings.zoom; 
            }
            DrawMenu();
            return; 
        }

        if (settings.zoom) {
            if ((held & BUTTON_L) && (held & BUTTON_ZR)) {
                WriteMemory<float>(MINECRAFT_FOV_ADDRESS, 30.0f); 
            } else {
                WriteMemory<float>(MINECRAFT_FOV_ADDRESS, 70.0f); 
            }
        }

        if (settings.antiExplosion) {
        }

        if (settings.villagerEditor) {
            if ((pressed & BUTTON_L) && (pressed & BUTTON_Y)) {
            }
        }

        if (settings.manaita) {
        }

        if (settings.roadMusic) {
        }
    }
};

extern "C" {
    struct VPADData {
        uint32_t btn_pressed;
        uint32_t btn_held;
    };

    void wps_process_frame(VPADData* vpad) {
        static AzeraClient client;
        if (vpad != nullptr) {
            client.OnFrameUpdate(vpad->btn_pressed, vpad->btn_held);
        }
    }
}
