#pragma once
#include "Countries.hpp"
#include <cmath>
namespace gt4 {
// Original procedural artwork. No Polyphony Digital texture rip is redistributed.
inline bool flagStar(float x,float y,float cx,float cy,float radius,float rotation=0){
 bool inside=false;float ax=0,ay=0,bx=0,by=0;for(int i=0,j=9;i<10;j=i++){
  auto point=[&](int k,float&px,float&py){const float a=rotation-1.57079633f+k*.62831853f,r=radius*(k%2?.382f:1.f);px=cx+std::cos(a)*r;py=cy+std::sin(a)*r;};point(i,ax,ay);point(j,bx,by);
  if((ay>y)!=(by>y)&&x<(bx-ax)*(y-ay)/(by-ay)+ax)inside=!inside;
 }return inside;
}
inline bool malaysiaStar(float x,float y){
 // Fourteen-point federal star, kept distinct from the five-point country stars.
 const float dx=(x+.25f)*1.5f,dy=y+.46f;
 const float angle=std::atan2(dy,dx)+1.57079633f;
 const float sector=std::remainder(angle,6.28318531f/14);
 const float edge=.12f/(std::cos(sector)+1.7f*std::abs(std::sin(sector)));
 return dx*dx+dy*dy<edge*edge;
}
inline uint32_t flagColor(Country country,float x,float y){ // nominal 3:2 flag, coordinates -1..1
 constexpr uint32_t white=0xFFFFFF,red=0xC8102E,blue=0x16366C;
 switch(country){
 case Country::Unknown:return white;
 case Country::Japan:return x*x+y*y<.29f?0xBC002D:white;
 case Country::Italy:return x<-.333f?0x009246:x>.333f?0xCE2B37:white;
 case Country::France:return x<-.333f?0x002395:x>.333f?0xED2939:white;
 case Country::Germany:return y<-.333f?0x151515:y>.333f?0xFFCE00:0xD00A14;
 case Country::Sweden:return std::abs(x+.28f)<.13f||std::abs(y)<.18f?0xFECC02:0x006AA7;
 case Country::UK:{const float diagonal=std::abs(std::abs(y)-std::abs(x));uint32_t c=blue;if(diagonal<.22f)c=white;if(diagonal<.075f)c=red;if(std::abs(x)<.20f||std::abs(y)<.25f)c=white;if(std::abs(x)<.11f||std::abs(y)<.14f)c=red;return c;}
 case Country::USA:{uint32_t c=int((y+1)*6.5f)%2?white:0xB22234;if(x<-.12f&&y<.077f){c=0x3C3B6E;for(int row=0;row<9;++row)for(int col=0;col<(row%2?5:6);++col)if(flagStar(x,y,-.94f+col*.145f+(row%2?.0725f:0),-.94f+row*.115f,.038f))return white;}return c;}
 case Country::Indonesia:return y<0?0xCE1126:white;
 case Country::Brazil:{if(std::abs(x)*.68f+std::abs(y)<.74f){if(x*x*2.25f+y*y<.30f){if(std::abs(y+.1f*x-.03f)<.065f)return white;for(const auto&p:std::array<std::array<float,2>,5>{{{{-.16f,.05f}},{{.13f,.19f}},{{.03f,-.15f}},{{-.03f,.28f}},{{.21f,-.03f}}}})if(flagStar(x,y,p[0],p[1],.025f))return white;return 0x002776;}return 0xFFDF00;}return 0x009C3B;}
 case Country::Malaysia:{uint32_t c=int((y+1)*7)%2?white:0xCC0001;if(x<.03f&&y<.06f){c=0x010066;const float a=(x+.52f)*(x+.52f)+(y+.47f)*(y+.47f),b=(x+.44f)*(x+.44f)+(y+.48f)*(y+.48f);if(a<.115f&&b>.082f)return 0xFFCC00;if(malaysiaStar(x,y))return 0xFFCC00;}return c;}
 case Country::Russia:return y<-.333f?white:y>.333f?0xD52B1E:0x0039A6;
 case Country::Denmark:return std::abs(x+.28f)<.12f||std::abs(y)<.17f?white:0xC8102E;
 case Country::Australia:{if(x<0&&y<0)return flagColor(Country::UK,x*2+1,y*2+1);if(flagStar(x,y,-.48f,.48f,.16f)||flagStar(x,y,.48f,-.50f,.12f)||flagStar(x,y,.70f,.20f,.11f)||flagStar(x,y,.26f,.14f,.11f)||flagStar(x,y,.48f,.68f,.13f)||flagStar(x,y,.58f,.38f,.055f))return white;return 0x012169;}
 case Country::Canada:{if(std::abs(x)>.55f)return red;const float points[][2]={{0,-.74f},{.12f,-.35f},{.29f,-.46f},{.25f,-.13f},{.47f,-.25f},{.39f,.07f},{.51f,.11f},{.19f,.36f},{.23f,.46f},{.035f,.42f},{.035f,.73f},{-.035f,.73f},{-.035f,.42f},{-.23f,.46f},{-.19f,.36f},{-.51f,.11f},{-.39f,.07f},{-.47f,-.25f},{-.25f,-.13f},{-.29f,-.46f},{-.12f,-.35f}};bool inside=false;for(size_t i=0,j=20;i<21;j=i++)if((points[i][1]>y)!=(points[j][1]>y)&&x<(points[j][0]-points[i][0])*(y-points[i][1])/(points[j][1]-points[i][1])+points[i][0])inside=!inside;return inside?red:white;}
 case Country::China:{if(flagStar(x,y,-.57f,-.45f,.24f))return 0xFFDE00;const float stars[][2]={{-.24f,-.72f},{-.10f,-.52f},{-.10f,-.29f},{-.26f,-.10f}};for(const auto&s:stars)if(flagStar(x,y,s[0],s[1],.075f,std::atan2(-.45f-s[1],-.57f-s[0])+1.57079633f))return 0xFFDE00;return 0xDE2910;}
 case Country::SouthKorea:{const float qx=x*1.5f,qy=y,r=.48f;if(qx*qx+qy*qy<r*r){const float boundary=qx<0?std::sqrt(std::max(0.f,r*r*.25f-(qx+r*.5f)*(qx+r*.5f))):-std::sqrt(std::max(0.f,r*r*.25f-(qx-r*.5f)*(qx-r*.5f)));return qy<boundary?0xCD2E3A:0x0047A0;}
  const float positions[][3]={{-.61f,-.61f,-.78539816f},{.61f,.61f,-.78539816f},{.61f,-.61f,.78539816f},{-.61f,.61f,.78539816f}};
  for(int g=0;g<4;++g){const auto&p=positions[g];float dx=(x-p[0])*1.5f,dy=y-p[1],u=dx*std::cos(p[2])+dy*std::sin(p[2]),v=-dx*std::sin(p[2])+dy*std::cos(p[2]);if(std::abs(u)<.29f)for(int line=0;line<3;++line)if(std::abs(v-(line-1)*.11f)<.034f){bool broken=g==1||(g==2&&line!=1)||(g==3&&line==1);if(!broken||std::abs(u)>.045f)return 0x161616;}}
  return white;}
 }return white;
}
inline uint32_t flagPixel(Country c,int px,int py,int size){
 unsigned a=0,r=0,g=0,b=0;for(int sy=0;sy<2;++sy)for(int sx=0;sx<2;++sx){float x=(px+(sx+.5f)*.5f)*2/size-1,y=(py+(sy+.5f)*.5f)*2/size-1,rad=std::sqrt(x*x+y*y);if(rad>1)continue;uint32_t rgb;if(rad>.91f)rgb=0x111819;else if(rad>.78f){const int shade=int(190-70*y);rgb=uint32_t(shade)*0x010101;}else{rgb=flagColor(c,x/.78f,y/.78f);if(c!=Country::Unknown&&rgb!=0xFFFFFF){const float light=1-.10f*std::max(0.f,y);auto channel=[&](int shift){return uint32_t(std::lround(float(rgb>>shift&255)*light));};rgb=(channel(16)<<16)|(channel(8)<<8)|channel(0);}}a+=255;r+=rgb>>16&255;g+=rgb>>8&255;b+=rgb&255;}
 return ((a/4)<<24)|(a?(r*255/a)<<16:0)|(a?(g*255/a)<<8:0)|(a?b*255/a:0);
}
}
