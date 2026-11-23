#pragma once
#include <array>
#include <cstdint>
namespace gt4 {
inline constexpr std::array<const char*,16> buttonArtworkNames{{"shared-normal","shared-highlight","purchase-normal","purchase-highlight","step-left-normal","step-left-highlight","step-right-normal","step-right-highlight","selector-down-normal","selector-down-highlight","selector-window-normal","selector-window-highlight","slider-normal","slider-highlight","transfender-back","prize-confirm"}};
inline constexpr std::array<uint32_t,16> buttonArtworkWidths{{384u,384u,384u,384u,80u,80u,80u,80u,80u,80u,80u,80u,16u,16u,1024u,384u}};
inline constexpr std::array<uint32_t,16> buttonArtworkHeights{{72u,72u,72u,72u,64u,64u,64u,64u,64u,64u,64u,64u,64u,64u,768u,72u}};
inline constexpr std::array<uint32_t,16> buttonArtworkChecksums{{2565923340u,3991142572u,3026296822u,11017263u,2479648937u,3200886789u,49724393u,3809288312u,3706942684u,2571426597u,2996771555u,133800909u,3510380962u,207102799u,3054492743u,1529231381u}};
}
