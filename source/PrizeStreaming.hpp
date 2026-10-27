#pragma once
#include "Prize.hpp"
namespace gt4 {
enum class PrizeStage {Model,Upgrades,Paint,Ready,Invalid};
inline const char* prizeStageName(PrizeStage stage){switch(stage){case PrizeStage::Model:return "vehicle";case PrizeStage::Upgrades:return "upgrades";case PrizeStage::Paint:return "paintjob";case PrizeStage::Ready:return "ready";default:return "invalid upgrade";}}
struct PrizeStreamReport {PrizeStage stage=PrizeStage::Model;int missing=-1,txd=-1,loaded=0,total=0;bool ready()const{return stage==PrizeStage::Ready;}};
inline std::vector<int> prizeDependencies(const PrizePlan& plan){auto result=prizeParts(plan);if(plan.kitResolved)return result;for(const auto& pair:prizeSkirtPairs)if(std::find(result.begin(),result.end(),pair.first)!=result.end())result.push_back(pair.second);return result;}
template<class Backend>PrizeStreamReport probePrize(const PrizePlan& plan,Backend& backend){
 PrizeStreamReport report;report.missing=plan.id;if(!plan)return report;
 backend.queueModel(plan.id);if(!backend.modelReady(plan.id))return report;
 const auto parts=prizeDependencies(plan);report.total=int(parts.size());report.missing=-1;
 for(int id:parts){if(!backend.validUpgrade(id)){report.stage=PrizeStage::Invalid;report.missing=id;return report;}backend.queueUpgrade(id);if(backend.upgradeReady(id))++report.loaded;else if(report.missing<0)report.missing=id;}
 report.txd=plan.enablePaintjob?backend.remap(plan):-1;const bool painted=report.txd<0||(backend.queueTxd(report.txd),backend.txdReady(report.txd));
 report.stage=report.missing>=0?PrizeStage::Upgrades:!painted?PrizeStage::Paint:PrizeStage::Ready;return report;
}
struct PrizeLoadGate {
 int id=-1;uint32_t serial=0;unsigned flushed=0;
 bool shouldFlush(const PrizePlan& plan,const PrizeStreamReport& report){if(id!=plan.id||serial!=plan.serial){id=plan.id;serial=plan.serial;flushed=0;}if(report.ready()||report.stage==PrizeStage::Invalid)return false;const unsigned bit=1u<<unsigned(report.stage);if(flushed&bit)return false;flushed|=bit;return true;}
};
}
