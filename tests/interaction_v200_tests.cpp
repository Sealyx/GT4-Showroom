#include "StudioHost.hpp"
#include <iostream>
using namespace gt4;
int main(){
 const auto folder=std::filesystem::temp_directory_path()/(L"GT4Showroom-v200-host-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(folder/L"GT4Showroom");base=folder.wstring()+L"\\";
 for(int n=0;n<4;++n){Entry e;e.id=400+n;e.name=std::string(1,char('D'-n));e.country=Country::USA;e.topSpeed=100+float(n*10);e.paints.resize(4);catalog.push_back(e);}entries=catalog;state.begin(entries.size());hostRelease();
 // Same production path: right click opens/closes only the context menu.
 hostKey(VK_RBUTTON);assert(state.ui.menu.open&&state.open&&!state.busy());hostKey(VK_RBUTTON);assert(!state.ui.menu.open&&state.open&&!state.busy());
 hostKey(VK_UP);assert(entries[state.selected].favorite);const int saved=entries[state.selected].id;hostKey(VK_UP);assert(entries[state.selected].favorite);hostKey('F');settleView();assert(state.ui.favoritesOnly&&entries.size()==1&&entries[0].id==saved&&state.open);
 hostKey(VK_DOWN);settleView();assert(entries.empty()&&state.open&&state.ui.favoritesOnly);hostKey(VK_DOWN);assert(entries.empty()&&!state.busy());hostKey(VK_BACK);settleView();assert(!state.ui.favoritesOnly&&entries.size()==4&&state.open);
 // Menu sort submenu and persisted order, keeping selection identity.
 hostKey(VK_RBUTTON);hostKey(VK_RIGHT);hostKey(VK_DOWN);hostKey(VK_DOWN);hostKey(VK_DOWN);hostKey(VK_RETURN);settleView();assert(state.ui.sortMode==3&&entries.front().topSpeed==130&&state.open);
 hostKey(VK_RBUTTON);hostKey(VK_DOWN);hostKey(VK_DOWN);hostKey(VK_RETURN);assert(state.ui.cinematic.active);const float a=state.ui.cinematic.angle(state.clock);hostFrame({},.3);assert(state.ui.cinematic.angle(state.clock)>a);hostKey(VK_SPACE);const float frozen=state.ui.cinematic.angle(state.clock);hostFrame({},.3);assert(state.ui.cinematic.angle(state.clock)==frozen);
 PadActions orbit;orbit.orbitX=1;orbit.zoom=1;hostFrame(orbit,.1);assert(state.ui.cinematic.angle(state.clock)>frozen&&state.ui.cinematic.zoomTarget>1.1f);hostKey(VK_BACK);assert(!state.ui.cinematic.active&&state.open);settleView();
 // Controller-only search, typing F via OSK does not trigger the Favorites shortcut.
 PadActions pad;pad.search=true;hostFrame(pad);hostRelease();assert(state.searching&&state.ui.keyboard.open);state.ui.keyboard.selected=5;pad={};pad.accept=true;hostFrame(pad);hostRelease();assert(state.searching&&state.query=="F");pad={};pad.back=true;hostFrame(pad);hostRelease();assert(!state.searching&&state.open);
 // Controller acceptance routes car -> paint -> spawn; Back returns paint -> cars.
 pad={};pad.accept=true;hostFrame(pad);hostRelease();assert(state.page==Page::Paint);pad={};pad.back=true;hostFrame(pad);hostRelease();assert(state.page==Page::Cars);hostKey(VK_DOWN);assert(!state.busy());
 assert(GetPrivateProfileIntW(L"View",L"SortMode",0,(folder/L"GT4Showroom/favorites.ini").c_str())==3);
 DeleteFileW((folder/L"GT4Showroom/favorites.ini").c_str());std::filesystem::remove(folder/L"GT4Showroom");std::filesystem::remove(folder);
 std::cout<<"PASS: production interaction routing for RMB-only sidebar, add/remove/no-op Favorites including empty view, black-frame filtered transitions, persisted speed sort, cinematic Space/controller orbit/zoom/back, controller OSK F and cancel, A/B paint paths. No GTA services invoked.\n";
}
