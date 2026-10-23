#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "Countries.hpp"
namespace gt4 {
struct UIRect{float x=0,y=0,w=0,h=0;bool contains(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
enum class StudioAction{None,SortAscending,SortDescending,SortClass,SortSpeed,SortCountry,SortModelId,Favorites,AllCars,RefreshFavorites,Cinematic,FilterCountry,ReloadSettings,FilterClass,Shadows,CarLogo,BrandLogo,Extra1,Extra2};
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
enum class MenuBranch{None,Sort,Countries,Classes,Shadows,Logos,BrandLogo,CarLogo,Extras,Extra1,Extra2};
struct MenuItem{std::string label;StudioAction action=StudioAction::None;int value=-1;MenuBranch child=MenuBranch::None;bool checked=false;};
inline constexpr size_t classCount=13;
inline constexpr const char* classNames[]={"Normal","Poor","Rich","Executive","Worker","Heavy","Taxi","Moped","Motorbike","Leisure Boat","Work Boat","Bicycle","Special"};
inline int classIndex(int value){return value>=0&&value<12?value:12;}
struct ContextMenu{
 // The three indices retain their historical names for saved UI fixtures.
 bool open=false,sub=false,countries=false,countryFocus=false;int row=0,sortRow=0,countryRow=0,choice=-1,focusLevel=0;float x=.5f,y=.5f;double began=0;
 std::vector<int> available{-1},classes{-1};std::array<int,countryCount> counts{};std::array<int,classCount> classCounts{};
 std::array<std::vector<std::string>,2> extras;std::array<int,2> extraChoice{{0,0}};bool favoriteView=false,cinematicView=false,shadows=true,carLogo=false,brandLogo=false;int order=0,countryFilter=-1,classFilter=-1;
 void sync(bool f,bool c,int o,int country,int cls,bool shadow,bool car,bool brand){favoriteView=f;cinematicView=c;order=o;countryFilter=country;classFilter=cls;shadows=shadow;carLogo=car;brandLogo=brand;}
 void setCountries(const std::array<int,countryCount>&c){if(counts==c)return;counts=c;available={-1};for(size_t i=1;i<countryCount;++i)if(c[i])available.push_back(int(i));if(c[0])available.push_back(0);countryRow=std::clamp(countryRow,0,int(available.size())-1);}
 void setClasses(const std::array<int,classCount>&c){if(classCounts==c)return;classCounts=c;classes={-1};for(size_t i=0;i<classCount;++i)if(c[i])classes.push_back(int(i));countryRow=std::clamp(countryRow,0,int(classes.size())-1);}
 void setExtras(std::vector<std::string> native,std::vector<std::string> vehfuncs){extras[0]=std::move(native);extras[1]=std::move(vehfuncs);row=std::min(row,int(rootItems().size())-1);}
 std::vector<MenuItem> rootItems()const{std::vector<MenuItem> r{{"Sort by",StudioAction::None,-1,MenuBranch::Sort},{favoriteView?"All Cars":"Favorites",favoriteView?StudioAction::AllCars:StudioAction::Favorites},{cinematicView?"Exit Cinematic":"Cinematic",StudioAction::Cinematic},{"Reload settings",StudioAction::ReloadSettings},{"Shadows",StudioAction::None,-1,MenuBranch::Shadows},{"Logos",StudioAction::None,-1,MenuBranch::Logos}};if(!extras[0].empty()||!extras[1].empty())r.push_back({"Extras",StudioAction::None,-1,MenuBranch::Extras});return r;}
 std::vector<MenuItem> branchItems(MenuBranch branch)const{
  switch(branch){
   case MenuBranch::Sort:return {{"Ascending  A - Z",StudioAction::SortAscending,0,MenuBranch::None,order==0},{"Descending  Z - A",StudioAction::SortDescending,1,MenuBranch::None,order==1},{"Class",StudioAction::None,-1,MenuBranch::Classes,classFilter>=0},{"Top Speed",StudioAction::SortSpeed,3,MenuBranch::None,order==3},{"Countries",StudioAction::None,-1,MenuBranch::Countries,countryFilter>=0},{"Model ID",StudioAction::SortModelId,5,MenuBranch::None,order==5}};
   case MenuBranch::Countries:{std::vector<MenuItem> r;for(int c:available){std::string label=c<0?"All Countries":c==0?"Unassigned":countryLabel(Country(c));if(c>=0)label+="  ("+std::to_string(counts[size_t(c)])+")";r.push_back({label,StudioAction::FilterCountry,c,MenuBranch::None,countryFilter==c});}return r;}
   case MenuBranch::Classes:{std::vector<MenuItem> r;for(int c:classes){std::string label=c<0?"All Classes":classNames[size_t(c)];if(c>=0)label+="  ("+std::to_string(classCounts[size_t(c)])+")";r.push_back({label,StudioAction::FilterClass,c,MenuBranch::None,classFilter==c});}return r;}
   case MenuBranch::Shadows:return {{"On",StudioAction::Shadows,1,MenuBranch::None,shadows},{"Off",StudioAction::Shadows,0,MenuBranch::None,!shadows}};
   case MenuBranch::Logos:return {{"Manufacturer",StudioAction::None,-1,MenuBranch::BrandLogo,brandLogo},{"Car name",StudioAction::None,-1,MenuBranch::CarLogo,carLogo}};
   case MenuBranch::BrandLogo:return {{"On",StudioAction::BrandLogo,1,MenuBranch::None,brandLogo},{"Off",StudioAction::BrandLogo,0,MenuBranch::None,!brandLogo}};
   case MenuBranch::CarLogo:return {{"On",StudioAction::CarLogo,1,MenuBranch::None,carLogo},{"Off",StudioAction::CarLogo,0,MenuBranch::None,!carLogo}};
   case MenuBranch::Extras:{std::vector<MenuItem> r;if(!extras[0].empty())r.push_back({"Extras 1",StudioAction::None,-1,MenuBranch::Extra1});if(!extras[1].empty())r.push_back({"Extras 2",StudioAction::None,-1,MenuBranch::Extra2});return r;}
   case MenuBranch::Extra1:case MenuBranch::Extra2:{const int group=branch==MenuBranch::Extra1?0:1;std::vector<MenuItem> r;for(size_t i=0;i<extras[size_t(group)].size();++i)r.push_back({extras[size_t(group)][i],group?StudioAction::Extra2:StudioAction::Extra1,int(i),MenuBranch::None,extraChoice[size_t(group)]==int(i)});return r;}
   default:return {};
  }
 }
 MenuBranch childBranch()const{const auto r=rootItems();return row>=0&&size_t(row)<r.size()?r[size_t(row)].child:MenuBranch::None;}
 MenuBranch thirdBranch()const{if(!sub)return MenuBranch::None;const auto r=branchItems(childBranch());return sortRow>=0&&size_t(sortRow)<r.size()?r[size_t(sortRow)].child:MenuBranch::None;}
 std::vector<MenuItem> items(int level)const{return level==0?rootItems():branchItems(level==1?childBranch():thirdBranch());}
 int current(int level)const{return level==0?row:level==1?sortRow:countryRow;}
 int depth()const{return !open?0:!sub?1:thirdBranch()==MenuBranch::None?2:3;}
 int focus()const{return countryFocus?2:focusLevel;}
 void path(){sub=childBranch()!=MenuBranch::None;const auto r=items(1);sortRow=r.empty()?0:std::clamp(sortRow,0,int(r.size())-1);countries=thirdBranch()==MenuBranch::Countries;const auto t=items(2);countryRow=t.empty()?0:std::clamp(countryRow,0,int(t.size())-1);if(t.empty())countryFocus=false;}
 void toggle(float px,float py,double now){open=!open;sub=countries=countryFocus=false;row=sortRow=countryRow=focusLevel=0;x=px;y=py;began=now;if(open)path();}
 int windowStart(int level,float h)const{const int count=int(items(level).size()),slots=std::max(1,int((h-30*(h/720.f))/(28*(h/720.f))));return std::clamp(current(level)-slots+1,0,std::max(0,count-slots));}
 std::array<UIRect,3> panels(float w,float h)const{
  const float s=h/720.f,gap=2*s;std::array<float,3> widths{{230*s,226*s,260*s}};const float full=widths[0]+widths[1]+widths[2]+2*gap;if(full>w-16*s){const float factor=std::max(.25f,(w-16*s-2*gap)/(widths[0]+widths[1]+widths[2]));for(auto&v:widths)v*=factor;}
  std::array<float,3> heights{};float maxHeight=0;for(int i=0;i<3;++i){heights[size_t(i)]=std::min(h-16*s,(14+28*std::max(size_t(1),items(i).size()))*s);maxHeight=std::max(maxHeight,heights[size_t(i)]);}const size_t reserve=std::max({rootItems().size(),size_t(6),available.size(),classes.size(),extras[0].size(),extras[1].size()});maxHeight=std::min(h-16*s,(14+28*reserve)*s);
  const float yy=std::clamp(y*h,8*s,std::max(8*s,h-maxHeight-8*s)),total=widths[0]+widths[1]+widths[2]+2*gap;
  // Every panel uses the same top edge. Reserve the complete chain up front.
  const float left=std::clamp(x*w,8*s,std::max(8*s,w-total-8*s));return {{{left,yy,widths[0],heights[0]},{left+widths[0]+gap,yy,widths[1],heights[1]},{left+widths[0]+widths[1]+2*gap,yy,widths[2],heights[2]}}};
 }
 UIRect panel(int level,float w,float h)const{return panels(w,h)[size_t(level)];}
 UIRect root(float w,float h)const{return panel(0,w,h);}UIRect child(float w,float h)const{return panel(1,w,h);}UIRect grandchild(float w,float h)const{return panel(2,w,h);}
 int hit(const UIRect&r,float px,float py,int rows,float s,int first=0)const{if(!r.contains(px,py))return -1;const int v=int((py-r.y-7*s)/(28*s))+first;return py>=r.y+7*s&&v>=first&&v<rows?v:-1;}
 void hover(float px,float py,float w,float h){const auto boxes=panels(w,h);for(int level=depth()-1;level>=0;--level){const int v=hit(boxes[size_t(level)],px,py,int(items(level).size()),h/720.f,windowStart(level,h));if(v<0)continue;if(level==2){countryRow=v;countryFocus=true;focusLevel=2;}else if(level==1){if(sortRow!=v)countryRow=0;sortRow=v;countryFocus=false;focusLevel=1;path();}else{if(row!=v){row=v;sortRow=countryRow=0;}countryFocus=false;focusLevel=0;path();}return;}}
 void move(int dx,int dy){if(!open)return;const int level=focus();if(level==2){if(dx<0){countryFocus=false;focusLevel=1;return;}const int n=int(items(2).size());if(n)countryRow=(countryRow+dy%n+n)%n;return;}
  if(level==1){if(dx<0){countryFocus=false;focusLevel=0;return;}const int n=int(items(1).size());if(n)sortRow=(sortRow+dy%n+n)%n;countries=thirdBranch()==MenuBranch::Countries;if(dy)countryRow=0;if(dx>0&&thirdBranch()!=MenuBranch::None){countryFocus=true;focusLevel=2;}return;}
  const int n=int(rootItems().size());row=(row+dy%n+n)%n;if(dy){sortRow=countryRow=0;path();}if(dx>0&&childBranch()!=MenuBranch::None){sub=true;focusLevel=1;countries=thirdBranch()==MenuBranch::Countries;}
 }
 StudioAction activate(bool favorites){favoriteView=favorites;if(!open)return StudioAction::None;int level=focus();auto r=items(level);if(r.empty())return StudioAction::None;const int idx=current(level);if(idx<0||size_t(idx)>=r.size())return StudioAction::None;const auto item=r[size_t(idx)];if(item.child!=MenuBranch::None){if(level==0){sub=true;focusLevel=1;}else{countryFocus=true;focusLevel=2;}countries=thirdBranch()==MenuBranch::Countries;return StudioAction::None;}choice=item.value;open=sub=countries=countryFocus=false;return item.action;}
 StudioAction click(float px,float py,float w,float h,bool favorites){hover(px,py,w,h);const auto b=panels(w,h);for(int level=depth()-1;level>=0;--level){const int n=hit(b[size_t(level)],px,py,int(items(level).size()),h/720.f,windowStart(level,h));if(n<0)continue;row=level==0?n:row;sortRow=level==1?n:sortRow;countryRow=level==2?n:countryRow;sub=level>0;countryFocus=level==2;focusLevel=level;return activate(favorites);}open=sub=countries=countryFocus=false;return StudioAction::None;}
};
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
 // 1 submit, -1 cancel, 0 continue editing.
 int type(std::string&query){if(selected==38)return 1;if(selected==39)return -1;if(selected==37){if(!query.empty())query.pop_back();}else if(query.size()<48)query+=selected==36?" ":key(selected);return 0;}
};
struct StudioUI{
 ContextMenu menu;CinematicView cinematic;ControllerKeyboard keyboard;ViewTransition transition;
 bool favoritesOnly=false,controllerUsed=false;int sortMode=5,countryFilter=-1,classFilter=-1;bool shadows=true,carLogo=false,brandLogo=false;double pointerMoved=0;bool dock=false;
 bool modal()const{return menu.open||keyboard.open;}
 void mouse(double now,float dx,float dy){if(std::abs(dx)+std::abs(dy)>.03f){pointerMoved=now;dock=false;controllerUsed=false;}}
 void resetTransient(){menu.open=false;keyboard.open=false;cinematic.leave();transition={};dock=false;}
};
}

