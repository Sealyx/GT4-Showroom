#include "StudioHost.hpp"
#include <iostream>
using namespace gt4;
int main(){
 catalog.resize(4);for(int i=0;i<4;++i){catalog[i].id=100+i*10;catalog[i].name=std::string(1,char('Z'-i));catalog[i].paints={Paint{},Paint{}};}
 catalog[0].country=Country::Japan;catalog[1].country=Country::USA;catalog[2].country=Country::Japan;catalog[3].country=Country::Unknown;
 state.begin(4);studioConfig.sort=menuSort(5);reloadExclusions();assert(entries.size()==4&&entries[0].id==100&&entries[3].id==130);
 assert(state.ui.menu.available.size()==4&&state.ui.menu.counts[size_t(Country::Japan)]==2);
 state.ui.menu.choice=int(Country::Japan);requestStudioAction(StudioAction::FilterCountry);settleView();assert(entries.size()==2&&entries[0].id==100&&entries[1].id==120);
 assert(state.ui.menu.counts[size_t(Country::USA)]==1); // list remains switchable after filtering
 state.ui.menu.choice=-1;requestStudioAction(StudioAction::FilterCountry);settleView();assert(entries.size()==4);
 requestStudioAction(StudioAction::Favorites);settleView();assert(entries.empty()&&state.selected==0&&state.count==0);
 state.move(1,entries);assert(!state.ui.dock);state.accept(entries);assert(!state.spawn&&state.page==Page::Cars);
 requestStudioAction(StudioAction::AllCars);settleView();assert(entries.size()==4);
 state.pointerX=.99f;state.pointerY=.05f;const float px=state.pointerX,py=state.pointerY;
 for(int i=0;i<900;++i)hostFrame();assert(state.pointerX==px&&state.pointerY==py&&!state.ui.dock);
 hostKey(VK_RIGHT);assert(state.selected==1&&state.pointerX<px&&state.ui.dock);
 const float moving=state.pointerX;PadActions pad;pad.right=true;hostFrame(pad);hostFrame(pad);assert(state.pointerX<moving);hostRelease();
 state.page=Page::Paint;state.paint=0;state.liveries={{-1,-1,{}},{0,20,{}},{1,21,{}},{3,23,{}}};state.liveriesModel=entries[state.selected].id;
 state.accept(entries);assert(state.page==Page::Livery&&!state.spawn&&state.livery==0);
 state.move(-1,entries);assert(state.livery==3&&state.ui.dock);state.move(1,entries);assert(state.livery==0);
 state.back();assert(state.page==Page::Paint);state.accept(entries);state.move(2,entries);state.accept(entries);assert(state.spawn&&state.livery==2);
 state.recovered();state.liveries={{-1,-1,{}}};state.page=Page::Paint;state.accept(entries);assert(state.spawn); // no native liveries skips the extra step
 for(auto dims:std::array<std::pair<float,float>,4>{{{800,600},{1280,720},{1920,1080},{3440,1440}}}){
  ContextMenu m;std::array<int,countryCount> n{};n.fill(1);m.setCountries(n);m.toggle(.98f,.99f,0);m.sub=true;m.sortRow=4;
  assert(m.activate(false)==StudioAction::None&&m.countries);auto r=m.child(dims.first,dims.second);assert(r.y>=0&&r.y+r.h<=dims.second);
  m.countryRow=2;const int choice=m.available[2];assert(m.activate(false)==StudioAction::FilterCountry&&m.choice==choice);
  m.toggle(.5f,.5f,0);m.sub=true;m.sortRow=5;assert(m.activate(false)==StudioAction::SortModelId);
 }
 std::cout<<"PASS: country availability/filter/clear; ID sorting; empty favorites; idle cursor stays put; keyboard/held controller browse docking; livery N+1/back/confirm/no-livery; country submenu at four resolutions.\n";
}
