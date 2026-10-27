#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "StudioUI.hpp"
namespace gt4 {
 
 
inline constexpr std::array<uint32_t,24> monitorOrange{{
 0xEDB55D,0xEEB65E,0xEFB65E,0xF0B75E,0xF0B75D,0xF0B75D,
 0xF0B85F,0xF0B85F,0xF1B75F,0xF2B75F,0xF2B75E,0xF2B75D,
 0xF2B75C,0xF2B555,0xEDAA41,0xE09B2F,0xBF7608,0xBF7706,
 0xC77E0B,0xCB810E,0xD38C18,0xDE9420,0xE29725,0xE29F39
}};
inline constexpr std::array<uint32_t,24> monitorArrowGray{{
 0xE6E9EF,0xE6E9EF,0xE6E7EE,0xE7E8EF,0xE7E9EF,0xE7E9EF,
 0xE7E9EF,0xE8EAF0,0xE8EAF0,0xE7E9EF,0xE6E7EF,0xE4E6EF,
 0xE2E4EC,0xDFE0E9,0xD6D8E1,0xC7C9D1,0xBBBDC6,0xBDBEC7,
 0xC0C2CA,0xC4C5CE,0xCCCDD5,0xD0D1D8,0xD2D3D9,0xD3D3D8
}};
inline constexpr std::array<uint32_t,24> monitorFieldGray{{
 0xBBBEC8,0xAFB3BB,0xA7ABB2,0xA3A4AB,0xA7A9B0,0xAAACB2,
 0xADAFB6,0xB1B3B9,0xB3B5BE,0xB7B8C1,0xB9BBC3,0xBBBDC6,
 0xBEBFC8,0xBEBFC8,0xC1C3CC,0xC2C4CD,0xC2C4CD,0xC4C5CE,
 0xC4C5CE,0xC6C7D0,0xC6C7D0,0xC7C9D1,0xC7C9D1,0xC5C6CF
}};
inline UIRect monitorArrowPanel(const UIRect&r,float scale){
 const float edge=std::max(.6f,scale),height=std::max(0.f,r.h-2*edge);
 const float width=std::min(std::max(0.f,r.w-2*edge),height*1.27f);
 return {r.x+r.w-edge-width,r.y+edge,width,height};
}
struct MonitorButtonImage {int width=0,height=0;std::vector<uint32_t> pixels;};
inline MonitorButtonImage monitorButtonImage(float aspect,bool selected){
  
  
 MonitorButtonImage out;const int body=64;out.width=std::clamp(int(std::lround(aspect*body)),128,1024);out.height=body+8;out.pixels.resize(size_t(out.width*out.height));
 const float radius=body*.5f;
 for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x){float a=0,r=0,g=0,b=0;
  for(int sy=0;sy<4;++sy)for(int sx=0;sx<4;++sx){const float px=x+(sx+.5f)/4,py=y+(sy+.5f)/4;
   const float center=std::clamp(px,radius,out.width-radius),dy=py-radius,dist=std::sqrt((px-center)*(px-center)+dy*dy);
   const float shadowY=py-radius-3,shadowDist=std::sqrt((px-center)*(px-center)+shadowY*shadowY);float opacity=0;uint32_t rgb=0;
   if(dist<=radius){const float u=(px-out.width*.5f)/(out.width*.5f),curve=.025f*(1-u*u);const float t=std::clamp(py/body+curve,0.f,.999f);
    rgb=selected?monitorOrange[size_t(t*monitorOrange.size())]:monitorArrowGray[size_t(t*monitorArrowGray.size())];
    if(dist>radius-1.1f)rgb=py<radius?(selected?0xFFE0A0u:0xFAFBFDu):(selected?0xC8913Bu:0x989AA3u);opacity=1;
   }else if(shadowDist<radius+4)opacity=.13f*std::clamp((radius+4-shadowDist)/4,0.f,1.f);
   a+=opacity;r+=((rgb>>16)&255)*opacity;g+=((rgb>>8)&255)*opacity;b+=(rgb&255)*opacity;
  }
  if(a>0)out.pixels[size_t(y*out.width+x)]=(uint32_t(std::lround(a*255/16))<<24)|(uint32_t(std::lround(r/a))<<16)|(uint32_t(std::lround(g/a))<<8)|uint32_t(std::lround(b/a));
 }
 return out;
}
}
