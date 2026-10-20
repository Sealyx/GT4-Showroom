#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>
#include "ModelFilter.hpp"
#include "SecretUnlocks.hpp"
#include "TuningPlan.hpp"
namespace gt4 {
enum class PrizeAbility {HeavyRamming,Proof,None};
inline const char* abilityName(PrizeAbility ability){return ability==PrizeAbility::None?"":ability==PrizeAbility::HeavyRamming?"HEAVY RAMMING":"BULLET / EXPLOSION PROTECTION";}
struct PrizePlan {int id=-1,style=0,paintjob=0,wheel=1073,paint=0;PrizeAbility ability=PrizeAbility::HeavyRamming;uint32_t serial=0;bool modkit=true,enablePaintjob=false,kitResolved=false,paintResolved=false;int remapTxd=-1;std::vector<int> upgrades;explicit operator bool()const{return id>=0;}};
struct PrizeTuner {int id;const char* model;};
inline constexpr std::array<PrizeTuner,5> prizeTuners={{{560,"sultan"},{562,"elegy"},{561,"stratum"},{565,"flash"},{559,"jester"}}};
template<class E>bool prizeTuner(const E& entry){for(const auto& tuner:prizeTuners)if(entry.id==tuner.id&&entry.modelKey==modelNameKey(tuner.model))return true;return false;}
template<class E>PrizePlan rollPrize(const std::vector<E>& entries,std::mt19937& random,const SecretUnlocksConfig& config){
 std::vector<size_t> available;for(size_t i=0;i<entries.size();++i)if(config.matches(entries[i]))available.push_back(i);
 if(available.empty())return {};auto pick=[&](int n){return std::uniform_int_distribution<int>(0,n-1)(random);};
 const auto& e=entries[available[size_t(pick(int(available.size())))]];
 static constexpr int wheels[]={1025,1073,1074,1075,1076,1077,1078,1079,1080,1081,1082,1083,1084,1085,1096,1097,1098};
 PrizePlan result;result.id=e.id;result.style=pick(2);result.paintjob=pick(3);result.wheel=wheels[pick(17)];result.paint=e.paints.empty()?0:pick(int(e.paints.size()));result.ability=config.enableAbilities?PrizeAbility(pick(2)):PrizeAbility::None;result.serial=random();result.modkit=config.enableModkit;result.enablePaintjob=config.enablePaintjob;return result;
}
template<class E>PrizePlan rollPrize(const std::vector<E>& entries,std::mt19937& random){SecretUnlocksConfig config;config.models={};parseModelTokens("560,562,561,565,559",config.models);config.enableAbilities=true;std::vector<E> tuners;for(const auto& entry:entries)if(prizeTuner(entry))tuners.push_back(entry);return rollPrize(tuners,random,config);}
inline std::string bonusLabel(PrizeAbility ability){const auto name=abilityName(ability);return *name?std::string("BONUS CAR FOUND  /  ")+name:"BONUS CAR FOUND";}
struct PrizeBody {int id;std::array<int,6> alien,xflow;};
#include "PrizeUpgrades.inc"
inline std::vector<int> prizeParts(const PrizePlan& plan){std::vector<int> result;if(!plan.modkit)return result;if(plan.kitResolved)return plan.upgrades;for(const auto& body:prizeBodies)if(body.id==plan.id){const auto& parts=plan.style?body.xflow:body.alien;result.assign(parts.begin(),parts.end());break;}if(result.empty())return result;result.insert(result.end(),{1010,1086,1087,plan.wheel});return result;}
inline float smoothFade(float value){value=std::clamp(value,0.f,1.f);return std::clamp(value*value*value*(value*(value*6-15)+10),0.f,1.f);}
struct PrizePose {float yaw=0,zoom=1,alpha=0,centerY=.52f;};
struct PrizeBackdrop {float center,wing,edge;PrizeBackdrop(float width,float height):center(std::min(width,height*4/3)),wing((width-center)*.5f),edge(std::min(.19f,wing/std::max(1.f,center))){} };
inline constexpr float prizeButtonTop=.895f,prizeButtonHeight=.034f;
inline float prizeButtonWidth(float width,float height){return std::min(width,height*4/3)*.235f;}
inline float prizeMeshScale(float span,float height){return std::min(5.1f/std::max(.01f,span),1.6f/std::max(.01f,height));}
// Source Congratulations.mp4: ghosted close frontal reveal, rapid pullback,
// then clockwise turn; the larger1.9.1 settled framing remains the destination.
inline constexpr double prizeNameTime=4.05,prizeCarTime=5.20,prizeIntroEnd=6.70,prizeReadyTime=8.0;
struct PrizeRevealKey{double time;float zoom,centerY,visibility;};
inline constexpr std::array<PrizeRevealKey,12> prizeRevealKeys={{{5.20,6.2f,1.05f,0},{5.30,5.3f,.94f,.025f},{5.40,4.3f,.85f,.103f},{5.50,3.4f,.76f,.161f},{5.60,2.8f,.71f,.211f},{5.80,2.05f,.64f,.314f},{6.00,1.55f,.59f,.409f},{6.20,1.25f,.55f,.504f},{6.50,1.03f,.522f,.661f},{6.70,1,.52f,.76f},{7.00,1,.52f,.909f},{7.20,1,.52f,1}}};
inline float prizeRevealValue(double time,float PrizeRevealKey::*field){
 const auto& k=prizeRevealKeys;if(time<=k.front().time)return k.front().*field;if(time>=k.back().time)return k.back().*field;
 auto slope=[&](size_t i){if(i==0)return float((k[1].*field-k[0].*field)/(k[1].time-k[0].time));if(i+1==k.size())return 0.f;const double a=k[i].time-k[i-1].time,b=k[i+1].time-k[i].time;const float u=float((k[i].*field-k[i-1].*field)/a),v=float((k[i+1].*field-k[i].*field)/b);if(u*v<=0)return 0.f;const double w1=2*b+a,w2=b+2*a;return float((w1+w2)/(w1/u+w2/v));};
 for(size_t i=1;i<k.size();++i)if(time<=k[i].time){const auto& a=k[i-1];const auto& b=k[i];const float h=float(b.time-a.time),u=float((time-a.time)/h),u2=u*u,u3=u2*u;const float value=(2*u3-3*u2+1)*(a.*field)+(u3-2*u2+u)*h*slope(i-1)+(-2*u3+3*u2)*(b.*field)+(u3-u2)*h*slope(i);return std::clamp(value,std::min(a.*field,b.*field),std::max(a.*field,b.*field));}return k.back().*field;
}
inline PrizePose prizePose(double time){const double age=std::isfinite(time)?std::max(0.,time):0;return {3.14159265358979323846f+float(std::max(0.,age-5.85))*float(6.28318530717958647692/20.4),prizeRevealValue(age,&PrizeRevealKey::zoom),prizeRevealValue(age,&PrizeRevealKey::visibility),prizeRevealValue(age,&PrizeRevealKey::centerY)};}
struct ReturnFade {static constexpr double duration=.12;double elapsed=duration;void begin(){elapsed=0;}void reset(){elapsed=duration;}void step(double delta){if(std::isfinite(delta))elapsed=std::clamp(elapsed+std::max(0.,delta),0.,duration);}bool active()const{return elapsed<duration;}float opacity()const{return 1-smoothFade(float(elapsed/duration));}};
// These fields belong to this spawned entity, never the shared handling table.
template<class Read,class Write>void applyPrizeAbility(PrizeAbility ability,Read read,Write write){
 if(ability==PrizeAbility::None)return;
 const uint32_t flags=uint32_t(read(0x40,true));
 if(ability==PrizeAbility::Proof){write(0x40,double(flags|0x00840000u),true);return;}
 write(0x40,double(flags|1u),true);
 for(const auto& item:std::array<std::pair<size_t,double>,3>{{{0x8C,3.0},{0x90,2.0},{0xA0,3.0}}}){const double value=read(item.first,false);if(std::isfinite(value)&&value>0&&value<1e9)write(item.first,value*item.second,false);}
}
}
