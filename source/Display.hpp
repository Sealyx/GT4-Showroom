#pragma once
#include "Showroom.hpp"

namespace gt4
{
    inline std::string vehicleCaption(const Entry& entry, const StudioUI& ui)
    {
        std::string caption = entry.name;
        if (!entry.year.empty()) {
            caption += " " + entry.year;
        }
        std::string metadata;
        if (ui.displayModelName && !entry.modelName.empty()) {
            metadata = entry.modelName;
        }
        if (ui.displayModelId) {
            if (!metadata.empty()) {
                metadata += ", ";
            }
            metadata += std::to_string(entry.id);
        }
        if (!metadata.empty()) {
            caption += " (" + metadata + ")";
        }
        if (entry.favorite) {
            caption += "  *";
        }
        return caption;
    }

    inline bool showroomReloadAllowed(bool showroomOpen,
                                      bool searching,
                                      bool focused,
                                      bool nativeMenu,
                                      bool codePause,
                                      int gameState,
                                      bool loading,
                                      bool cutscene,
                                      bool modifier)
    {
        return !showroomOpen && !searching && focused && !nativeMenu && !codePause && gameState == 9 && !loading &&
               !cutscene && !modifier;
    }

    inline constexpr const char* showroomReloadMessage = "Gran Turismo Spawner reloaded";

    inline bool controllerReloadRequested(bool open, bool select, bool leftStick)
    {
        return !open && select && leftStick;
    }
}
