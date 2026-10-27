#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace gt4 {
struct ArrowPoint{float x,y;};
inline constexpr ArrowPoint pointerTilt(ArrowPoint p){return {.866025404f*p.x+.5f*p.y,-.5f*p.x+.866025404f*p.y};}
inline constexpr std::array<ArrowPoint,4> cursorOutline={{pointerTilt({0,0}),pointerTilt({-14,32}),pointerTilt({0,25.5f}),pointerTilt({14,32})}};
inline constexpr std::array<ArrowPoint,4> cursorInset={{pointerTilt({0,2.5f}),pointerTilt({-12.1f,29.5f}),pointerTilt({0,23.9f}),pointerTilt({12.1f,29.5f})}};
static_assert(14*14+32*32==(-14)*(-14)+32*32,"Equal rear leg lengths before rigid rotation.");
inline constexpr std::array<ArrowPoint,5> thumbnailPeak={{{0,-4.5f},{-1.3f,-4.2f},{-12,3.5f},{12,3.5f},{1.3f,-4.2f}}};
inline float thumbnailPulse(double elapsed){
 if(!std::isfinite(elapsed))elapsed=0;
 const double phase=std::fmod(std::max(0.,elapsed),1.2)/1.2;
 return float(.5+.5*std::cos(phase*6.28318530717958647692));
}
}



