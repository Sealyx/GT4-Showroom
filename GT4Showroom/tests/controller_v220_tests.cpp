#include "StudioHost.hpp"
#include "../source/KeyboardInput.hpp"
#include <iostream>
using namespace gt4;
static void tap(PadActions p){hostRelease();hostFrame(p);hostRelease();}
int main(){
 XINPUT_GAMEPAD raw{};raw.wButtons=XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_BACK;auto xp=xinputController(raw);assert(xp.pause&&xp.view);
 ControllerInput native;native.values[0x18/2]=128;native.values[0x20/2]=128;auto merged=combineControllers(nativeController(native),xinputController(XINPUT_GAMEPAD{}));assert(merged.pause&&merged.accept);
 assert(studioOpenInput(false,false,xp)&&!studioOpenInput(false,true,xp)&&studioOpenInput(true,true,{}));
 Key opening;assert(opening.poll(studioOpenInput(false,false,xp),1));assert(!opening.poll(true,2));assert(!opening.poll(false,3));assert(opening.poll(true,4));
 opening.blockUntilRelease();assert(!opening.poll(true,5));assert(!opening.poll(false,6));assert(opening.poll(true,7)); // Held View cannot reopen on exit.
 OpenGate gate;gate.gameState=9;gate.focus=gate.alive=true;assert(gate.ready());gate.inVehicle=true;assert(!gate.ready());gate.inVehicle=false;gate.cutscene=true;assert(!gate.ready());
 std::array<uint8_t,0x270> now{},old{};now.fill(0x55);old.fill(0x66);auto a=now,b=old;
 {ScopedEscapePulse pulse(now.data(),old.data(),true);assert(nativeKeyDown(now.data(),VK_ESCAPE));int16_t z;std::memcpy(&z,old.data()+0x218,2);assert(z==0);}
 assert(now==a&&old==b);
 {ScopedEscapePulse pulse(now.data(),old.data(),true);int16_t consumed=0;std::memcpy(now.data()+0x218,&consumed,2);}assert(!nativeKeyDown(now.data(),VK_ESCAPE)); // native clear wins
 const auto folder=std::filesystem::temp_directory_path()/(L"GT4-controller220-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(folder/L"GT4Showroom");base=folder.wstring()+L"\\";
 for(int i=0;i<3;++i){Entry e;e.id=560+i;e.name="Car"+std::to_string(i);e.paints.resize(3);e.country=i==0?Country::SouthKorea:Country::Japan;catalog.push_back(e);}entries=catalog;state.begin(entries.size());hostRelease();refreshCountryMenu();
 PadActions p;p.view=true;tap(p);assert(state.ui.menu.open);p={};p.down=true;for(int i=0;i<3;++i)tap(p);p={};p.accept=true;tap(p);assert(hostReloads==1&&!state.ui.menu.open);
 p={};p.view=true;tap(p);p={};p.right=true;tap(p);p={};p.down=true;for(int i=0;i<4;++i)tap(p);p={};p.accept=true;tap(p);assert(state.ui.menu.countries);p={};p.down=true;tap(p);p={};p.accept=true;tap(p);settleView();assert(state.ui.countryFilter==int(Country::Japan)&&entries.size()==2);
 p={};p.add=true;tap(p);assert(entries[state.selected].favorite);p={};p.favorites=true;tap(p);settleView();assert(state.ui.favoritesOnly&&entries.size()==1);p={};p.back=true;tap(p);settleView();assert(!state.ui.favoritesOnly);
 p={};p.search=true;tap(p);assert(state.searching&&state.ui.keyboard.open);state.ui.keyboard.selected=0;p={};p.accept=true;tap(p);assert(state.query=="A");p={};p.add=true;tap(p);assert(state.query.empty());p={};p.back=true;tap(p);assert(!state.searching);
 p={};p.accept=true;tap(p);assert(state.page==Page::Paint);state.liveries={{-1,-1,{}},{0,0,{}}};state.liveriesModel=entries[state.selected].id;tap(p);assert(state.page==Page::Livery);p={};p.right=true;tap(p);assert(state.livery==1);p={};p.back=true;tap(p);assert(state.page==Page::Paint);tap(p);assert(state.page==Page::Cars);
 p={};p.view=true;tap(p);p={};p.down=true;tap(p);tap(p);p={};p.accept=true;tap(p);assert(state.ui.cinematic.active);p={};p.accept=true;tap(p);assert(state.ui.cinematic.paused);p={};p.orbitX=1;p.zoom=1;hostFrame(p,.1);assert(state.ui.cinematic.zoomTarget>1.1f);p={};p.back=true;tap(p);assert(!state.ui.cinematic.active);
 assert(std::string(countryLabel(Country::SouthKorea))=="South Korea");assert(std::string(countryKeys[size_t(Country::SouthKorea)])=="SouthKorea");
 DeleteFileW((folder/L"GT4Showroom/favorites.ini").c_str());std::filesystem::remove(folder/L"GT4Showroom");std::filesystem::remove(folder);
 std::cout<<"PASS controller220: opening/gates/held edges; idle XInput/native merge; owned Esc pulse/native clear; pad-only reload/country/favorites/OSK/paint/livery/cinematic; South Korea label with legacy INI key. No GTA hardware session.\n";
}
