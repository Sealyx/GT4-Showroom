#pragma once
#include "Showroom.hpp"
namespace gt4 {
inline constexpr float prizeNameY=.278f;
// destination Y / original reference Y; the title band is unchanged.
inline constexpr std::array<std::pair<float,float>,5> prizeBackdropBands={{{0,0},{.35f,.35f},{.66f,.74f},{.91f,.91f},{1,1}}};
inline Camera prizeCamera(float width,float height,const PrizePose& pose){Camera camera(width,height,5.f,pose.centerY);const V3 target{0,0,.75f};camera.eye={0,-7.4f,2.0f};camera.forward=unit(target-camera.eye);camera.right=unit(cross(camera.forward,{0,0,1}));camera.up=cross(camera.right,camera.forward);camera.focal*=pose.zoom;return camera;}

}
