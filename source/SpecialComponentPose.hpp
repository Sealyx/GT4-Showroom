#pragma once
namespace gt4 {
// Exact non-emitting subset of SA 1.0 US CAutomobile::PreRender's Cement524
// branch (0x6AC43B..0x6AC4D4). The exported DFF stores both pivots untilted.
// The engine applies -10 degrees around X; misc_b also rotates around Y.
// A static showroom uses its zero-spin pose, keeping the stock support tilt.
template<class Node,class Rotate>
bool prepareCementPose(void*vehicle,int model,Node node,Rotate rotate){
 if(model!=524)return false;constexpr float tilt=-.1745329350233078f;
 if(void*a=node(20))rotate(vehicle,a,0,tilt,true);
 if(void*b=node(21)){rotate(vehicle,b,1,0.f,true);rotate(vehicle,b,0,tilt,false);}return true;
}
}
