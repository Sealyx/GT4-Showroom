#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "Countries.hpp"
#include "AudioLevels.hpp"
#include "CinematicVehicle.hpp"
namespace gt4 {
struct UIRect{float x=0,y=0,w=0,h=0;bool contains(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
enum class StudioAction{None,SortAscending,SortDescending,SortClass,SortSpeed,SortCountry,SortModelId,Favorites,AllCars,RefreshFavorites,Cinematic,FilterCountry,FilterClass,Shadows,CarLogo,BrandLogo,Extra1,Extra2,DisplayModelId,DisplayModelName,Hotspots,Lights,Indicators,VolumeMaster,VolumeMusic,VolumeEffects};
struct ViewTransition{
 StudioAction action=StudioAction::None;int phase=0;double began=0;bool black=false;
 bool active()const{return phase!=0;}
 bool request(StudioAction a,double now){if(active())return false;action=a;phase=1;began=now;black=false;return true;}
 float alpha(double now)const{const float t=float((now-began)/.16);const float x=std::clamp(t,0.f,1.f),v=x*x*(3-2*x);return phase==1?v:phase==2?1.f:phase==3?1-v:0;}
 void step(double now){if(phase==1&&now-began>=.16)phase=2;else if(phase==3&&now-began>=.16){phase=0;action=StudioAction::None;black=false;}}
 void presented(double now){if(phase==2||(phase==1&&now-began>=.16))black=true;}
 bool ready()const{return phase==2&&black;}
 void applied(double now){phase=3;began=now;}
};
enum class MenuBranch{None,Sort,Countries,Classes,Shadows,Logos,BrandLogo,CarLogo,Extras,Extra1,Extra2,Display,DisplayModelId,DisplayModelName,Cinematic,Favorites,CinematicControls,Hotspots,Lights,Indicators,Volume};
struct MenuItem{std::string label;StudioAction action=StudioAction::None;int value=-1;MenuBranch child=MenuBranch::None;bool checked=false;};
inline constexpr size_t classCount=13;
inline constexpr const char* classNames[]={"Normal","Poor","Rich","Executive","Worker","Heavy","Taxi","Moped","Motorbike","Leisure Boat","Work Boat","Bicycle","Special"};
inline int classIndex(int value){return value>=0&&value<12?value:12;}
struct ContextMenu{
 bool monitorPicker=false,monitorHover=false,monitorHeaderHover=false,monitorKeyboard=false;int monitorDragLevel=-1,monitorHelpLevel=-1,monitorHelpIndex=-1,monitorHoverLevel=-1,monitorHoverIndex=-1,monitorFooter=-1,monitorFooterLevel=0;float monitorDragX=0,monitorDragY=0;std::array<float,3> monitorX{},monitorY{};std::array<bool,3> monitorPlaced{};double monitorChanged=0;int closeResult=0,sliderDrag=-1;bool sliderChanged=false;AudioLevels volumes;bool hotspots=true,lights=false;int indicators=0;
 bool open=false,sub=false,countries=false,countryFocus=false;int row=0,sortRow=0,countryRow=0,choice=-1,focusLevel=0;float x=.5f,y=.5f;double began=0;
 std::vector<int> available{-1},classes{-1};std::array<int,countryCount> counts{};std::array<int,classCount> classCounts{};
 std::array<std::vector<std::string>,2> extras;std::array<int,2> extraChoice{{0,0}};bool favoriteView=false,cinematicView=false,shadows=true,carLogo=false,brandLogo=false,displayModelId=false,displayModelName=false;int order=0,countryFilter=-1,classFilter=-1;
 void sync(bool f,bool c,int o,int country,int cls,bool shadow,bool car,bool brand,bool showId=false,bool showName=false){favoriteView=f;cinematicView=c;order=o;countryFilter=country;classFilter=cls;shadows=shadow;carLogo=car;brandLogo=brand;displayModelId=showId;displayModelName=showName;}
 void setCountries(const std::array<int,countryCount>& c) {
  if(counts==c)return;
  const int focused=thirdBranch()==MenuBranch::Countries&&countryRow>=0&&size_t(countryRow)<available.size()?available[size_t(countryRow)]:countryFilter;
  counts=c;available={-1};
  for(size_t i=0;i<countryCount;++i)if(c[i])available.push_back(int(i));
  std::sort(available.begin()+1,available.end(),[&](int a,int b){
   if(c[size_t(a)]!=c[size_t(b)])return c[size_t(a)]>c[size_t(b)];
   const std::string an=a?countryLabel(Country(a)):"Unassigned",bn=b?countryLabel(Country(b)):"Unassigned";
   return an<bn;
  });
  if(thirdBranch()==MenuBranch::Countries){const auto i=std::find(available.begin(),available.end(),focused);countryRow=i==available.end()?0:int(i-available.begin());}
 }
 void setClasses(const std::array<int,classCount>& c) {
  if(classCounts==c)return;
  const int focused=thirdBranch()==MenuBranch::Classes&&countryRow>=0&&size_t(countryRow)<classes.size()?classes[size_t(countryRow)]:classFilter;
  classCounts=c;classes={-1};
  for(size_t i=0;i<classCount;++i)if(c[i])classes.push_back(int(i));
  std::sort(classes.begin()+1,classes.end(),[&](int a,int b){return c[size_t(a)]!=c[size_t(b)]?c[size_t(a)]>c[size_t(b)]:std::string(classNames[size_t(a)])<classNames[size_t(b)];});
  if(thirdBranch()==MenuBranch::Classes){const auto i=std::find(classes.begin(),classes.end(),focused);countryRow=i==classes.end()?0:int(i-classes.begin());}
 }
 void setExtras(const std::vector<std::string>& native,const std::vector<std::string>& vehfuncs){if(extras[0]==native&&extras[1]==vehfuncs)return;extras[0]=native;extras[1]=vehfuncs;row=std::min(row,int(rootItems().size())-1);}
 std::vector<MenuItem> rootItems()const {
  std::vector<MenuItem> rows{{"Sort by",StudioAction::None,-1,MenuBranch::Sort},
   {"Favorites",StudioAction::None,-1,MenuBranch::Favorites},
   {"Cinematic",StudioAction::None,-1,MenuBranch::Cinematic},
   {"Display",StudioAction::None,-1,MenuBranch::Display},
   {"Shadows",StudioAction::None,-1,MenuBranch::Shadows},
   {"Logos",StudioAction::None,-1,MenuBranch::Logos}};
  if(!extras[0].empty()||!extras[1].empty())rows.push_back({"Extras",StudioAction::None,-1,MenuBranch::Extras});
  rows.push_back({"Volume",StudioAction::None,-1,MenuBranch::Volume});
  if(cinematicView)rows.push_back({"Cinematic controls",StudioAction::None,-1,MenuBranch::CinematicControls});
  return rows;
 }
 std::vector<MenuItem> branchItems(MenuBranch branch)const{
  switch(branch){
   case MenuBranch::Sort:return {{"Ascending  A - Z",StudioAction::SortAscending,0,MenuBranch::None,order==0},{"Descending  Z - A",StudioAction::SortDescending,1,MenuBranch::None,order==1},{"Class",StudioAction::None,-1,MenuBranch::Classes,classFilter>=0},{"Top Speed",StudioAction::SortSpeed,3,MenuBranch::None,order==3},{"Countries",StudioAction::None,-1,MenuBranch::Countries,countryFilter>=0},{"Model ID",StudioAction::SortModelId,5,MenuBranch::None,order==5}};
   case MenuBranch::Countries:{std::vector<MenuItem> r;for(int c:available){std::string label=c<0?"All Countries":c==0?"Unassigned":countryLabel(Country(c));if(c>=0)label+="  ("+std::to_string(counts[size_t(c)])+")";r.push_back({label,StudioAction::FilterCountry,c,MenuBranch::None,countryFilter==c});}return r;}
   case MenuBranch::Classes:{std::vector<MenuItem> r;for(int c:classes){std::string label=c<0?"All Classes":classNames[size_t(c)];if(c>=0)label+="  ("+std::to_string(classCounts[size_t(c)])+")";r.push_back({label,StudioAction::FilterClass,c,MenuBranch::None,classFilter==c});}return r;}
   case MenuBranch::Cinematic:return {{"On",StudioAction::Cinematic,1,MenuBranch::None,cinematicView},{"Off",StudioAction::Cinematic,0,MenuBranch::None,!cinematicView}};
   case MenuBranch::Favorites:return {{"All Cars",StudioAction::AllCars,0,MenuBranch::None,!favoriteView},{"Favorites",StudioAction::Favorites,1,MenuBranch::None,favoriteView}};
   case MenuBranch::CinematicControls:return {{"Hotspots",StudioAction::None,-1,MenuBranch::Hotspots,hotspots},{"Lights",StudioAction::None,-1,MenuBranch::Lights,lights},{"Indicators",StudioAction::None,-1,MenuBranch::Indicators,indicators!=0}};
   case MenuBranch::Hotspots:return {{"On",StudioAction::Hotspots,1,MenuBranch::None,hotspots},{"Off",StudioAction::Hotspots,0,MenuBranch::None,!hotspots}};
   case MenuBranch::Lights:return {{"On",StudioAction::Lights,1,MenuBranch::None,lights},{"Off",StudioAction::Lights,0,MenuBranch::None,!lights}};
   case MenuBranch::Indicators:return {{"Off",StudioAction::Indicators,0,MenuBranch::None,indicators==0},{"Left",StudioAction::Indicators,1,MenuBranch::None,indicators==1},{"Right",StudioAction::Indicators,2,MenuBranch::None,indicators==2},{"Hazard",StudioAction::Indicators,3,MenuBranch::None,indicators==3}};
   case MenuBranch::Volume:return {{"Master",StudioAction::VolumeMaster,volumes.value[0]},{"Music",StudioAction::VolumeMusic,volumes.value[1]},{"Sound effects",StudioAction::VolumeEffects,volumes.value[2]}};
   case MenuBranch::Shadows:return {{"On",StudioAction::Shadows,1,MenuBranch::None,shadows},{"Off",StudioAction::Shadows,0,MenuBranch::None,!shadows}};
   case MenuBranch::Logos:return {{"Manufacturer",StudioAction::None,-1,MenuBranch::BrandLogo,brandLogo},{"Car name",StudioAction::None,-1,MenuBranch::CarLogo,carLogo}};
   case MenuBranch::BrandLogo:return {{"On",StudioAction::BrandLogo,1,MenuBranch::None,brandLogo},{"Off",StudioAction::BrandLogo,0,MenuBranch::None,!brandLogo}};
   case MenuBranch::CarLogo:return {{"On",StudioAction::CarLogo,1,MenuBranch::None,carLogo},{"Off",StudioAction::CarLogo,0,MenuBranch::None,!carLogo}};
   case MenuBranch::Display:return {{"Model ID",StudioAction::None,-1,MenuBranch::DisplayModelId,displayModelId},{"Model name",StudioAction::None,-1,MenuBranch::DisplayModelName,displayModelName}};
   case MenuBranch::DisplayModelId:return {{"On",StudioAction::DisplayModelId,1,MenuBranch::None,displayModelId},{"Off",StudioAction::DisplayModelId,0,MenuBranch::None,!displayModelId}};
   case MenuBranch::DisplayModelName:return {{"On",StudioAction::DisplayModelName,1,MenuBranch::None,displayModelName},{"Off",StudioAction::DisplayModelName,0,MenuBranch::None,!displayModelName}};
   case MenuBranch::Extras:{std::vector<MenuItem> r;if(!extras[0].empty())r.push_back({"Vanilla",StudioAction::Extra1,extraChoice[0]});if(!extras[1].empty())r.push_back({"VehFuncs",StudioAction::Extra2,extraChoice[1]});return r;}
   case MenuBranch::Extra1:case MenuBranch::Extra2:{const int group=branch==MenuBranch::Extra1?0:1;std::vector<MenuItem> r;for(size_t i=0;i<extras[size_t(group)].size();++i)r.push_back({extras[size_t(group)][i],group?StudioAction::Extra2:StudioAction::Extra1,int(i),MenuBranch::None,extraChoice[size_t(group)]==int(i)});return r;}
   default:return {};
  }
 }
 MenuBranch childBranch()const{const auto r=rootItems();return row>=0&&size_t(row)<r.size()?r[size_t(row)].child:MenuBranch::None;}
 MenuBranch thirdBranch()const{if(!sub)return MenuBranch::None;const auto branch=childBranch();if((branch==MenuBranch::Logos||branch==MenuBranch::Extras||branch==MenuBranch::Display||branch==MenuBranch::CinematicControls||branch==MenuBranch::Volume)&&focus()==0)return MenuBranch::None;const auto r=branchItems(branch);return sortRow>=0&&size_t(sortRow)<r.size()?r[size_t(sortRow)].child:MenuBranch::None;}
 std::vector<MenuItem> items(int level)const{return level==0?rootItems():branchItems(level==1?childBranch():thirdBranch());}
 std::array<int,2> previewExtras()const{auto selected=extraChoice;if(open&&focus()==2){const auto branch=thirdBranch();if(branch==MenuBranch::Extra1||branch==MenuBranch::Extra2){const size_t group=branch==MenuBranch::Extra2?1:0;if(countryRow>=0&&size_t(countryRow)<extras[group].size())selected[group]=countryRow;}}return selected;}
 int current(int level)const{return level==0?row:level==1?sortRow:countryRow;}
 int depth()const{return !open?0:!sub?1:thirdBranch()==MenuBranch::None?2:3;}
 int focus()const{return countryFocus?2:focusLevel;}
 void path(){sub=childBranch()!=MenuBranch::None;const auto r=items(1);sortRow=r.empty()?0:std::clamp(sortRow,0,int(r.size())-1);countries=thirdBranch()==MenuBranch::Countries;const auto t=items(2);countryRow=t.empty()?0:std::clamp(countryRow,0,int(t.size())-1);if(t.empty())countryFocus=false;}
 void toggle(float px,float py,double now){monitorToggle(px,py,now);}
 int windowStart(int level,float h)const{return monitorStart(level,h);}
 std::array<UIRect,3> panels(float w,float h)const{return monitorPanels(w,h);}
 UIRect panel(int level,float w,float h)const{return monitorPanels(w,h)[size_t(level)];}
 UIRect root(float w,float h)const{return panel(0,w,h);}
 UIRect child(float w,float h)const{return panel(1,w,h);}
 UIRect grandchild(float w,float h)const{return panel(2,w,h);}
 void hover(float px,float py,float w,float h){monitorPoint(px,py,w,h);}
 void move(int dx,int dy){monitorMove(dx,dy);}
 StudioAction activate(bool favorites){return monitorActivate(favorites);}
 StudioAction click(float px,float py,float w,float h,bool favorites){return monitorClick(px,py,w,h,favorites);}
 bool monitorSubWindow()const{const auto b=childBranch();return b==MenuBranch::Logos||b==MenuBranch::Extras||b==MenuBranch::Display||b==MenuBranch::Volume||b==MenuBranch::CinematicControls;}
 bool monitorSelectorWindow()const{return sub&&focus()==2&&!monitorSubWindow();}
 void monitorToggle(float px,float py,double now);
 int monitorStart(int level,float h)const;
 std::array<UIRect,3> monitorPanels(float w,float h)const;
 UIRect monitorField(int level,int index,float w,float h)const;
 UIRect monitorDropdown(float w,float h)const;
 UIRect monitorButton(int level,int which,float w,float h)const;
 UIRect monitorListRow(UIRect list,int level,int index,float h)const;
 UIRect monitorSlider(int index,float w,float h)const;
 bool monitorSlide(float px,float py,float w,float h,bool down,bool pressed);
 bool sliderWindow()const{return childBranch()==MenuBranch::Volume||childBranch()==MenuBranch::Extras;}
 int extraGroup(int index)const{const auto rows=items(1);return index>=0&&size_t(index)<rows.size()?(rows[size_t(index)].action==StudioAction::Extra1?0:rows[size_t(index)].action==StudioAction::Extra2?1:-1):-1;}
 int sliderMaximum(int index)const{const int group=extraGroup(index);return childBranch()==MenuBranch::Volume?100:group>=0?std::max(0,int(extras[size_t(group)].size())-1):0;}
 int sliderValue(int index)const{const int group=extraGroup(index);return childBranch()==MenuBranch::Volume&&index>=0&&index<3?volumes.value[size_t(index)]:group>=0?extraChoice[size_t(group)]:0;}
 bool setSliderValue(int index,int value){value=std::clamp(value,0,sliderMaximum(index));const int group=extraGroup(index);int*target=childBranch()==MenuBranch::Volume&&index>=0&&index<3?&volumes.value[size_t(index)]:group>=0?&extraChoice[size_t(group)]:nullptr;if(!target||*target==value)return false;*target=value;sliderChanged=true;return true;}
 std::string sliderLabel(int index)const{if(childBranch()==MenuBranch::Volume)return std::to_string(sliderValue(index))+"%";const int group=extraGroup(index);return group>=0&&!extras[size_t(group)].empty()?extras[size_t(group)][size_t(std::clamp(sliderValue(index),0,sliderMaximum(index)))]:"Default";}
 bool fieldOpensWindow(int level,int index)const;
 int audibleTarget()const;
 void finish(int result){closeResult=result;open=sub=countries=countryFocus=monitorPicker=false;sliderDrag=-1;}
 bool monitorFieldSelected(int level,int index,bool single=false)const;
 bool monitorButtonSelected(int level,int which)const;
 UIRect pointerTarget(float w,float h)const;
 std::string monitorValue(int level,int index)const;
 std::string monitorTitle(int level)const;
 std::string help()const;
 void monitorPoint(float px,float py,float w,float h);
 void monitorMove(int dx,int dy);
 void monitorBack();
 StudioAction monitorActivate(bool favorites);
 StudioAction monitorClick(float px,float py,float w,float h,bool favorites);
 bool monitorDrag(float px,float py,float w,float h,bool down,bool pressed);
 void selectedAction(){monitorPicker=false;monitorFooter=-1;if(focus()==2){countryFocus=false;focusLevel=monitorSubWindow()?1:0;sub=monitorSubWindow();}}
};
#include "MonitorSettings.inl"
struct CinematicView{
 bool active=false,paused=false,grabbed=false,dragged=false;float mix=0,yaw=0,pitch=.23f,zoom=1,zoomTarget=1;double began=0,lightHeld=0,lightBegan=0;float grabX=0,grabY=0;
 float angle(double now)const{return yaw+(!paused?float(now-began)*.314159265f:0.f);}
 double lightTime(double now)const{return lightHeld+(!paused?now-lightBegan:0.);}
 void enter(double now,float angle){active=true;paused=grabbed=dragged=false;yaw=angle;began=lightBegan=now;lightHeld=now;pitch=.23f;zoomTarget=1.10f;}
 void leave(){active=grabbed=false;zoomTarget=1;}
 void toggle(double now){yaw=angle(now);lightHeld=lightTime(now);began=lightBegan=now;paused=!paused;}
 void orbit(float dx,float dy,double now){if(std::abs(dx)+std::abs(dy)<.0001f)return;yaw=angle(now)+dx;lightHeld=lightTime(now);began=lightBegan=now;paused=true;pitch=std::clamp(pitch+dy,.03f,1.15f);}
 void wheel(float v){zoomTarget=std::clamp(zoomTarget*std::exp(v*.10f),.55f,2.4f);}
 void step(double dt){const float t=float(1-std::exp(-dt*12));mix+=((active?1.f:0.f)-mix)*t;zoom+=(zoomTarget-zoom)*t;}
};
struct ControllerKeyboard{
 bool open=false;int selected=0;
 static std::string key(int i){static const std::string chars="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";return i<36?chars.substr(size_t(i),1):i==36?"SPACE":i==37?"DEL":i==38?"FIND":"CLOSE";}
 void move(int dx,int dy){selected=((selected/10+dy+4)%4)*10+(selected%10+dx+10)%10;}
 UIRect box(float w,float h,int i)const{const float s=h/720.f,cw=48*s,ch=37*s;return {(w-10*cw)*.5f+(i%10)*cw,h*.68f+(i/10)*ch,cw-3*s,ch-3*s};}
 int type(std::string&query){if(selected==38)return 1;if(selected==39)return -1;if(selected==37){if(!query.empty())query.pop_back();}else if(query.size()<48)query+=selected==36?" ":key(selected);return 0;}
};
struct StudioUI{
 ContextMenu menu;CinematicView cinematic,menuCinematic;CinematicVehicleState vehicle,menuVehicle;AudioLevels audioLevels,menuAudio;bool menuSnapshot=false;ControllerKeyboard keyboard;ViewTransition transition;
 bool favoritesOnly=false,controllerUsed=false;int sortMode=5,countryFilter=-1,classFilter=-1;bool shadows=true,carLogo=false,brandLogo=false,displayModelId=false,displayModelName=false;double pointerMoved=0;bool dock=false;
 bool modal()const{return menu.open||keyboard.open;}
 void mouse(double now,float dx,float dy){if(std::abs(dx)+std::abs(dy)>.03f){pointerMoved=now;dock=false;controllerUsed=false;}}
 void resetTransient(){menu.open=false;keyboard.open=false;cinematic.leave();vehicle.reset();menuSnapshot=false;transition={};dock=false;}
};
}
