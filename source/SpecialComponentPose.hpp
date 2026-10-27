#pragma once
namespace gt4 {
 
 
 
 
template<class Node,class Rotate>
bool prepareCementPose(void*vehicle,int model,Node node,Rotate rotate){
 if(model!=524)return false;constexpr float tilt=-.1745329350233078f;
 if(void*a=node(20))rotate(vehicle,a,0,tilt,true);
 if(void*b=node(21)){rotate(vehicle,b,1,0.f,true);rotate(vehicle,b,0,tilt,false);}return true;
}
}
