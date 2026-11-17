#pragma once
#include "InputState.hpp"
#include <windows.h>

namespace gt4
{
    enum class SearchAction
    {
        None,
        Submit,
        Cancel
    };

    struct SearchInput
    {
        std::array<Key, 256> keys{};

        void quarantine()
        {
            for (auto& k : keys) {
                k.blockUntilRelease();
            }
        }

        template <class Down>
        SearchAction poll(Down down, uint64_t tick, std::string& text, bool allowEdit = true, size_t limit = 48,
                          bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0,
                          HKL layout = GetKeyboardLayout(0))
        {
            SearchAction action = SearchAction::None;
            std::array<BYTE, 256> keyboard{};
            for (size_t vk = 0; vk < keyboard.size(); ++vk) {
                keyboard[vk] = down(int(vk)) ? 0x80 : 0;
            }
            keyboard[VK_CAPITAL] |= caps ? 1 : 0;
            keyboard[VK_NUMLOCK] |= GetKeyState(VK_NUMLOCK) & 1;
            keyboard[VK_SHIFT] |= keyboard[VK_LSHIFT] | keyboard[VK_RSHIFT];
            keyboard[VK_CONTROL] |= keyboard[VK_LCONTROL] | keyboard[VK_RCONTROL];
            keyboard[VK_MENU] |= keyboard[VK_LMENU] | keyboard[VK_RMENU];
            for (int vk = 8; vk < 256; ++vk) {
                const bool event = keys[size_t(vk)].poll(down(vk), tick, vk == 8 || vk == 0x2E);
                if (!event) {
                    continue;
                }
                if (vk == 13) {
                    action = SearchAction::Submit;
                    continue;
                }
                if (vk == 8) {
                    if (!allowEdit) {
                        action = SearchAction::Cancel;
                        continue;
                    }
                    if (down(0x11)) {
                        text.clear();
                    } else if (!text.empty()) {
                        size_t first = text.size() - 1;
                        while (first && (static_cast<unsigned char>(text[first]) & 0xC0) == 0x80) {
                            --first;
                        }
                        text.erase(first);
                    } else {
                        action = SearchAction::Cancel;
                    }
                    continue;
                }
                if (vk == 0x2E) {
                    if (allowEdit) {
                        text.clear();
                    }
                    continue;
                }
                if (down(0x11) || down(0x12)) {
                    continue;
                }
                if (!allowEdit || text.size() >= limit) {
                    continue;
                }
                WCHAR translated[8]{};
                const UINT scan = MapVirtualKeyExW(UINT(vk), MAPVK_VK_TO_VSC, layout);
                const int count = ToUnicodeEx(UINT(vk), scan, keyboard.data(), translated, 8, 4, layout);
                if (count <= 0 || translated[0] < 32 || translated[0] == 127) {
                    continue;
                }
                char encoded[32]{};
                const int bytes = WideCharToMultiByte(CP_UTF8, 0, translated, count, encoded, sizeof(encoded),
                                                       nullptr, nullptr);
                if (bytes > 0 && text.size() + size_t(bytes) <= limit) {
                    text.append(encoded, size_t(bytes));
                }
            }
            return action;
        }
    };
}
