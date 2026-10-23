#pragma once
#include "ModelNames.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace gt4 {
inline float adaptivePreviewSpan(float actualSpan,bool compact){
 if(!std::isfinite(actualSpan)||actualSpan<=.01f)return 5.f;
 const float target=std::clamp(5.f*std::pow(actualSpan/4.7f,.22f),3.3f,6.8f);
 return compact?target*.84f:target;
}
inline unsigned paintSlotCount(uint8_t mask){return (mask&8)?4u:(mask&4)?3u:2u;}
inline bool alphaCutout(bool textureAlpha,float materialAlpha){return textureAlpha&&materialAlpha>=.98f;}
}
