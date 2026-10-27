#pragma once
#include "Showroom.hpp"
namespace gt4 {
inline std::vector<std::string> vehicleDisplayRows(const Entry&e,const StudioUI&ui){
 std::vector<std::string> rows;if(ui.displayModelId)rows.push_back("MODEL ID  "+std::to_string(e.id));
 if(ui.displayModelName)rows.push_back(e.modelName.empty()?"MODEL NAME UNKNOWN":e.modelName);
 return rows;
}
template<class Box>inline UIRect vehicleDisplayPanel(float w,float h,const Box&car,size_t rows){
 const float scale=h/720.f,width=std::min(w*.38f,280*scale),height=(12+22*rows)*scale;
 return {std::clamp(car.x+car.w*.5f-width*.5f,12*scale,std::max(12*scale,w-width-12*scale)),std::clamp(car.y-height-10*scale,100*scale,std::max(100*scale,h*.64f-height)),width,height};
}
inline bool showroomReloadAllowed(bool showroomOpen,bool searching,bool focused,bool nativeMenu,bool codePause,int gameState,bool loading,bool cutscene,bool modifier){
 return !showroomOpen&&!searching&&focused&&!nativeMenu&&!codePause&&gameState==9&&!loading&&!cutscene&&!modifier;
}
inline constexpr const char* showroomReloadMessage="Gran Turismo Spawner reloaded.";
inline bool controllerReloadRequested(bool open,bool select,bool leftStick){return !open&&select&&leftStick;}
}
