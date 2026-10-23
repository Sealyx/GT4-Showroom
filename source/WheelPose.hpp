#pragma once
namespace gt4 {
using WheelMatrixUpdate=void(__thiscall*)(void*,int,int);
template<class Node>bool prepareWheelPose(void* vehicle,int kind,int model,Node node,WheelMatrixUpdate update){if(kind!=0&&kind!=1&&kind!=2&&kind!=11)return false;for(int index:{2,5,4,7,3,6})if(node(index))update(vehicle,index,model==432?7:0);return true;}
}
