#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include "Countries.hpp"
namespace gt4 {
struct UIRect{float x=0,y=0,w=0,h=0;bool contains(float a,float b)const{return a>=x&&b>=y&&a<x+w&&b<y+h;}};
enum class StudioAction{None,SortAscending,SortDescending,SortClass,SortSpeed,SortCountry,SortModelId,Favorites,AllCars,RefreshFavorites,Cinematic,FilterCountry,ReloadSettings};
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
struct ContextMenu{
 bool open=false,sub=false,countries=false;int row=0,sortRow=0,countryRow=0,choice=-1;float x=.5f,y=.5f;double began=0;
 std::vector<int> available{-1};std::array<int,countryCount> counts{};
 void setCountries(const std::array<int,countryCount>&c){if(counts==c)return;counts=c;available={-1};for(size_t i=1;i<countryCount;++i)if(c[i])available.push_back(int(i));if(c[0])available.push_back(0);countryRow=std::clamp(countryRow,0,int(available.size())-1);}
 void toggle(float px,float py,double now){open=!open;sub=countries=false;row=sortRow=countryRow=0;x=px;y=py;began=now;}
 UIRect root(float w,float h)const{const float s=h/720.f,wide=230*s,tall=126*s;return {std::clamp(x*w,8*s,std::max(8*s,w-wide-8*s)),std::clamp(y*h,8*s,std::max(8*s,h-tall-8*s)),wide,tall};}
 UIRect child(float w,float h)const{const auto r=root(w,h);const float s=h/720.f,wide=(countries?260:226)*s,tall=(14+28*(countries?available.size():6))*s;const float xx=r.x+r.w+wide+8*s<=w?r.x+r.w+2*s:std::max(8*s,r.x-wide-2*s);return {xx,std::clamp(r.y+3*s,8*s,std::max(8*s,h-tall-8*s)),wide,tall};}
 int hit(const UIRect&r,float px,float py,int rows,float s)const{if(!r.contains(px,py))return -1;const int v=int((py-r.y-7*s)/(28*s));return py>=r.y+7*s&&v>=0&&v<rows?v:-1;}
 void hover(float px,float py,float w,float h){const float s=h/720.f;const int n=hit(root(w,h),px,py,4,s);if(n>=0){row=n;sub=n==0;if(n!=0)countries=false;}if(sub){const int q=hit(child(w,h),px,py,countries?int(available.size()):6,s);if(q>=0){if(countries)countryRow=q;else sortRow=q;}}}
 void move(int dx,int dy){if(sub){if(dx<0){if(countries)countries=false;else sub=false;return;}if(countries)countryRow=(countryRow+dy+int(available.size()))%int(available.size());else{sortRow=(sortRow+dy+6)%6;if(dx>0&&sortRow==4)countries=true;}}else{row=(row+dy+4)%4;if(dx>0&&row==0)sub=true;}}
 StudioAction activate(bool favorites){if(!open)return StudioAction::None;if(row==0&&!sub){sub=true;return StudioAction::None;}if(row==0&&sortRow==4&&!countries){countries=true;return StudioAction::None;}StudioAction a;if(row==0&&countries){choice=available[size_t(countryRow)];a=StudioAction::FilterCountry;}else a=row==0?StudioAction(int(StudioAction::SortAscending)+sortRow):row==1?(favorites?StudioAction::AllCars:StudioAction::Favorites):row==2?StudioAction::Cinematic:StudioAction::ReloadSettings;open=false;sub=countries=false;return a;}
 StudioAction click(float px,float py,float w,float h,bool favorites){const auto r=root(w,h),c=child(w,h);const float s=h/720.f;if(sub){const int n=hit(c,px,py,countries?int(available.size()):6,s);if(n>=0){if(countries)countryRow=n;else sortRow=n;row=0;return activate(favorites);}}const int n=hit(r,px,py,4,s);if(n>=0){row=n;if(n==0){sub=true;return StudioAction::None;}return activate(favorites);}open=sub=countries=false;return StudioAction::None;}
};
struct CinematicView{
 bool active=false,paused=false,grabbed=false,dragged=false;float mix=0,yaw=0,pitch=.23f,zoom=1,zoomTarget=1;double began=0;float grabX=0,grabY=0;
 float angle(double now)const{return yaw+(!paused?float(now-began)*.314159265f:0.f);}
 void enter(double now,float angle){active=true;paused=grabbed=dragged=false;yaw=angle;began=now;pitch=.23f;zoomTarget=1.10f;}
 void leave(){active=grabbed=false;zoomTarget=1;}
 void toggle(double now){yaw=angle(now);began=now;paused=!paused;}
 void orbit(float dx,float dy,double now){if(std::abs(dx)+std::abs(dy)<.0001f)return;yaw=angle(now)+dx;began=now;paused=true;pitch=std::clamp(pitch+dy,-.06f,1.15f);}
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
 bool favoritesOnly=false,controllerUsed=false;int sortMode=0,countryFilter=-1;double pointerMoved=0;bool dock=false;
 bool modal()const{return menu.open||keyboard.open;}
 void mouse(double now,float dx,float dy){if(std::abs(dx)+std::abs(dy)>.03f){pointerMoved=now;dock=false;controllerUsed=false;}}
 void resetTransient(){menu.open=false;keyboard.open=false;cinematic.leave();transition={};dock=false;}
};
}
