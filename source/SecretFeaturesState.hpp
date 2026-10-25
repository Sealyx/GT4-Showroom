#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace gt4 {
struct FailRect {float x=0,y=0,w=0,h=0;bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}};
inline float failSmooth(float v){v=std::clamp(v,0.f,1.f);return v*v*(3-2*v);}
struct SecretFeaturesLayout {
 FailRect scene,label,word,buttons[2];float labelAlpha=0,wordAlpha=0,buttonAlpha=0,backgroundAlpha=1,buttonTextHeight=0;
 SecretFeaturesLayout(float w,float h,double age){
  // Measured from the supplied1920x1080 reference. Native viewport, no4:3 frame.
  const float t=failSmooth(float((age-.10)/.55));auto mix=[&](float a,float b){return a+(b-a)*t;};
  scene={mix(0,w*.0990f),mix(0,h*.2565f),mix(w,w*.4675f),mix(h,h*.4710f)};
  label={w*(1166.f/1920),h*(315.f/1080),h*(214.f/1080),h*(40.f/1080)};word={w*(1168.f/1920),h*(409.f/1080),h*(432.f/1080),h*(117.f/1080)};
  buttons[0]={w*(1178.f/1920),h*(671.f/1080),w*(434.f/1920),h*(50.f/1080)};buttons[1]={w*(1178.f/1920),h*(742.f/1080),w*(434.f/1920),h*(50.f/1080)};
  labelAlpha=failSmooth(float((age-.63)/.19));wordAlpha=failSmooth(float((age-.93)/.24));buttonAlpha=failSmooth(float((age-2.05)/.20));buttonTextHeight=h*.0235f;
 }
 int hit(float x,float y)const{if(buttonAlpha<.99f)return -1;for(int i=0;i<2;++i)if(buttons[i].contains(x,y))return i;return -1;}
};
struct SecretFeaturesInput {bool up=false,down=false,accept=false,back=false,click=false;bool mouseMoved=false;float mouseX=0,mouseY=0;};
enum class SecretFeaturesChoice {None,Retry,Exit};
enum class FailureKind {School,Wasted,Busted};
struct FailureText {const char*label;const char*word;};
inline FailureText failureText(FailureKind kind){switch(kind){case FailureKind::Wasted:return {"Wasted","YOU DIED"};case FailureKind::Busted:return {"Busted","ARRESTED"};default:return {"License","FAIL"};}}
class SecretFeaturesState {
 bool previousUp=false,previousDown=false,previousAccept=false,previousBack=false,previousClick=false;bool quarantine=true,docking=false;
public:
 bool active=false,accepted=false;int selected=0,test=1;double age=0;float pointerX=0,pointerY=0;bool pointerInitialized=false;uint64_t serial=0;FailureKind kind=FailureKind::School;
 void begin(int n,FailureKind k=FailureKind::School){active=true;accepted=false;selected=0;test=std::clamp(n,1,16);kind=k;age=0;quarantine=true;pointerInitialized=false;docking=false;++serial;}
 void reset(){active=accepted=false;kind=FailureKind::School;age=0;quarantine=true;docking=false;pointerInitialized=false;previousUp=previousDown=previousAccept=previousBack=previousClick=false;}
 bool interactive()const{return true;}
 bool ready()const{return interactive()&&active&&!accepted&&age>=2.25;}
 void suspended(){quarantine=true;}
 SecretFeaturesChoice update(double dt,const SecretFeaturesInput&i,float w,float h){
  if(!active)return SecretFeaturesChoice::None;age+=std::clamp(dt,0.,.10);if(!interactive())return SecretFeaturesChoice::None;const SecretFeaturesLayout l(w,h,age);
  if(!pointerInitialized){pointerX=l.buttons[0].x+l.buttons[0].w*.5f;pointerY=l.buttons[0].y+l.buttons[0].h*.5f;pointerInitialized=true;}
  const bool any=i.up||i.down||i.accept||i.back||i.click;
  const bool u=i.up&&!previousUp,d=i.down&&!previousDown,a=i.accept&&!previousAccept,b=i.back&&!previousBack,c=i.click&&!previousClick;
  previousUp=i.up;previousDown=i.down;previousAccept=i.accept;previousBack=i.back;previousClick=i.click;
  if(quarantine){if(!any)quarantine=false;return SecretFeaturesChoice::None;}
  if(i.mouseMoved){pointerX=std::clamp(i.mouseX,0.f,w);pointerY=std::clamp(i.mouseY,0.f,h);const int hit=l.hit(pointerX,pointerY);if(hit>=0){selected=hit;docking=false;}}
  // Dock only after an actual keyboard/controller action, never from an idle timer.
  if(u||d||a||b){docking=true;if(u||d)selected=1-selected;}if(docking){const auto&r=l.buttons[selected];const float blend=1-std::exp(-24.f*float(std::max(0.,dt)));pointerX+=(r.x+r.w*.5f-pointerX)*blend;pointerY+=(r.y+r.h*.5f-pointerY)*blend;}
  if(!ready())return SecretFeaturesChoice::None;
  if(b){selected=1;accepted=true;return SecretFeaturesChoice::Exit;}
  if(a||(c&&l.hit(pointerX,pointerY)>=0)){if(c)selected=l.hit(pointerX,pointerY);accepted=true;return selected?SecretFeaturesChoice::Exit:SecretFeaturesChoice::Retry;}
  return SecretFeaturesChoice::None;
 }
};
}
