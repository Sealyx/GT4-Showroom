#include "../source/Presentation.hpp"
#include "../source/StudioConfig.hpp"
#include "../source/Controller.hpp"
#include "../source/SnapshotPolicy.hpp"
#include "../source/CountryFlags.hpp"
#include <cassert>
#include <iostream>
using namespace gt4;
int main(){
 std::vector<Entry> entries(3);entries[0].id=1;entries[0].name="Zulu";entries[0].country=Country::USA;entries[0].topSpeed=100;entries[1].id=2;entries[1].name="Alpha";entries[1].country=Country::Brazil;entries[1].topSpeed=200;entries[2].id=3;entries[2].name="Middle";entries[2].country=Country::Unknown;entries[2].topSpeed=-1;
 sortCatalogue(entries,menuSort(0));assert(entries.front().name=="Alpha");sortCatalogue(entries,menuSort(1));assert(entries.front().name=="Zulu");sortCatalogue(entries,menuSort(3));assert(entries.front().topSpeed==200&&entries.back().topSpeed==-1);sortCatalogue(entries,menuSort(4));assert(entries.front().country==Country::Brazil&&entries.back().country==Country::Unknown);
 for(auto dims:std::array<std::pair<float,float>,5>{{{800,600},{1280,720},{1920,1080},{2560,1080},{5120,1440}}}){
  ContextMenu menu;menu.toggle(.98f,.98f,1);const auto r=menu.root(dims.first,dims.second),c=menu.child(dims.first,dims.second);assert(r.x>=0&&r.x+r.w<=dims.first&&r.y+r.h<=dims.second);assert(c.x>=0&&c.x+c.w<=dims.first&&c.y+c.h<=dims.second);menu.move(1,0);menu.move(0,3);assert(menu.activate(false)==StudioAction::SortSpeed&&!menu.open);
  menu.toggle(.3f,.3f,1);menu.row=1;assert(menu.activate(false)==StudioAction::Favorites);menu.toggle(.3f,.3f,1);menu.row=1;assert(menu.activate(true)==StudioAction::AllCars);
 }
 ViewTransition transition;assert(transition.request(StudioAction::Favorites,0));transition.step(.17);assert(transition.alpha(.17)==1&&!transition.ready());transition.presented(.17);assert(transition.ready());transition.applied(.17);transition.step(.34);assert(!transition.active());
 CinematicView view;view.enter(10,2);assert(std::abs(view.angle(11)-(2+.314159265f))<.0001f);view.toggle(11);const float stopped=view.angle(11);assert(view.angle(30)==stopped);view.orbit(.2f,4,30);assert(view.paused&&view.pitch==1.15f);view.wheel(100);assert(view.zoomTarget==2.4f);view.wheel(-100);assert(view.zoomTarget==.55f);view.toggle(31);assert(!view.paused&&view.angle(32)>view.angle(31));view.step(.3);assert(view.mix>.9f);view.leave();view.step(.3);assert(view.mix<.1f);
 Model state;state.begin(entries.size());state.clock=3;state.ui.dock=true;state.pointerX=.99f;state.pointerY=.1f;for(int i=0;i<50;++i)dockPointer(state,entries,1280,720,1./60);const float x=state.pointerX,y=state.pointerY;assert(x<.8f&&y>.7f);state.pointerMove(100,-100,1280,720);const float mouseX=state.pointerX;dockPointer(state,entries,1280,720,.2);assert(state.pointerX==mouseX);state.clock+=1.1;dockPointer(state,entries,1280,720,.2);assert(state.pointerX==mouseX);
 state.ui.menu.open=true;const float still=state.pointerX;dockPointer(state,entries,1280,720,1);assert(state.pointerX==still);
 ControllerKeyboard keyboard;std::string text;keyboard.selected=5;assert(keyboard.type(text)==0&&text=="F");keyboard.selected=33;keyboard.type(text);assert(text=="F7");keyboard.selected=37;keyboard.type(text);assert(text=="F");keyboard.selected=38;assert(keyboard.type(text)==1);keyboard.selected=39;assert(keyboard.type(text)==-1);keyboard.selected=0;keyboard.move(-1,-1);assert(keyboard.selected==39);
 XINPUT_GAMEPAD raw{};raw.wButtons=XINPUT_GAMEPAD_BACK|XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_X|XINPUT_GAMEPAD_Y|XINPUT_GAMEPAD_LEFT_THUMB;raw.sThumbRX=32767;raw.sThumbRY=-32768;raw.bRightTrigger=255;const auto pad=xinputController(raw);assert(pad.view&&pad.accept&&pad.add&&pad.favorites&&pad.search&&pad.orbitX==1&&pad.orbitY==1&&pad.zoom==1);
 ControllerInput native;native.values[0x1A/2]=1;native.values[0x20/2]=1;native.values[0x24/2]=1;assert(nativeController(native).view&&nativeController(native).accept&&nativeController(native).search);assert(deadAxis(10,128,28)==0);
 for(const auto*frame:{"chassis_vlo","door_lf_dam","body_lod_1","body_LO","part_low2"})assert(!previewHighDetail(frame));for(const auto*frame:{"body","door_lf_ok","lowrider_body","f_wheel","f_lowering"})assert(previewHighDetail(frame));assert(!previewHighDetail("body",0x7331E0));assert(textureAddresses(0x1106)==std::make_pair(1u,1u)&&textureAddresses(0x2306)==std::make_pair(3u,2u));
 Entry rumpo;rumpo.id=440;assert(defaultCountry(rumpo)==Country::USA);assert(flagColor(Country::Indonesia,0,-.5f)==0xCE1126&&flagColor(Country::Indonesia,0,.5f)==0xFFFFFF);assert(std::string(countryKeys[size_t(Country::Brazil)])=="Brazil"&&std::string(countryKeys[size_t(Country::Malaysia)])=="Malaysia");
 std::vector<std::array<uint16_t,3>> decoded;auto emit=[&](uint16_t a,uint16_t b,uint16_t c){decoded.push_back({a,b,c});};
 const uint16_t list[]={0,1,2,2,3,0,9,1,2,0,0,1,0};assert(previewTriangles(list,13,0,4,50,emit)==2&&decoded[0]==(std::array<uint16_t,3>{0,1,2}));
 decoded.clear();const uint16_t strip[]={0,1,2,3,3,4,4,5,6};assert(previewTriangles(strip,9,1,7,50,emit)==3);assert(decoded[1]==(std::array<uint16_t,3>{2,1,3})&&decoded[2]==(std::array<uint16_t,3>{4,5,6}));
 decoded.clear();assert(previewTriangles(strip,9,1,7,1,emit)==1&&decoded.size()==1);assert(previewTriangles(strip,9,2,7,10,emit)==0&&previewTriangles(nullptr,9,0,7,10,emit)==0);
 std::cout<<"PASS: menu layout/actions at five resolutions; presented-black catalogue transitions; cinematic auto/pause/orbit/zoom; browse-only pointer return; controller keyboard and XInput/native maps; country/speed sorting; HD frame/texture policy; Rumpo and new country badges.\n";
}
