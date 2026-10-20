#define GTA_BINDING_TEST
#include "../source/NativePrize.hpp"
#include <cassert>
#include <filesystem>
#include <map>
#include <set>
#include <cstdio>
using namespace gt4;
struct FakeBackend {
 int car=561,txd=42,carReads=0,remapReads=0,txdRequests=0;bool carLoaded=false,paintLoaded=false;std::set<int> loaded,requested;int invalid=-1;
 void queueModel(int id){assert(id==car);}bool modelReady(int id){assert(id==car);++carReads;return carLoaded;}
 bool validUpgrade(int id){return id!=invalid;}void queueUpgrade(int id){requested.insert(id);}bool upgradeReady(int id){return loaded.count(id)!=0;}
 int remap(const PrizePlan&){++remapReads;return txd;}void queueTxd(int id){assert(id==txd);++txdRequests;}bool txdReady(int id){assert(id==txd);return paintLoaded;}
};
int main(){
 Entry baggage;baggage.id=485;baggage.modelKey=modelNameKey("baggage");baggage.name="Baggage";baggage.paints.resize(3);
 Entry stratum;stratum.id=561;stratum.modelKey=modelNameKey("stratum");stratum.name="Stratum";stratum.paints.resize(4);
 Entry addon;addon.id=20042;addon.modelKey=modelNameKey("lucent");addon.name="Lucent";Entry sultan;sultan.id=560;sultan.modelKey=modelNameKey("sultan");
 const std::vector<Entry> cars{baggage,stratum,addon,sultan};SecretUnlocksConfig config;config.models={};parseModelTokens("485,stratum",config.models);config.enableAbilities=false;std::mt19937 random{42};std::set<int> selected;
 for(int i=0;i<2000;++i){const auto plan=rollPrize(cars,random,config);selected.insert(plan.id);assert(plan.id==485||plan.id==561);assert(plan.ability==PrizeAbility::None);assert(plan.modkit);assert(prizeParts(plan).size()==(plan.id==561?10u:0u));}
 assert(selected.size()==2);config.models={};parseModelTokens("485, STRATUM ,LUCENT,485",config.models);config.enableAbilities=true;selected.clear();std::set<PrizeAbility> abilities;
 for(int i=0;i<10000;++i){const auto plan=rollPrize(cars,random,config);assert(plan.id!=560&&plan.ability!=PrizeAbility::None);selected.insert(plan.id);abilities.insert(plan.ability);if(plan.id==20042)assert(plan.modkit);}
 assert(selected.size()==3&&abilities.size()==2);config.enableModkit=false;for(int i=0;i<50;++i)assert(!rollPrize(cars,random,config).modkit);
 config.models={};assert(!rollPrize(cars,random,config));Model menu;menu.secretUnlocks=config;menu.begin(cars.size());menu.beginSearch();menu.query="REALBIG";menu.submitSearch(cars);assert(menu.searchFailed&&menu.page==Page::Cars&&!menu.prize);
 assert(bonusLabel(PrizeAbility::None)=="BONUS CAR FOUND"&&bonusLabel(PrizeAbility::HeavyRamming)=="BONUS CAR FOUND  /  HEAVY RAMMING");int touches=0;applyPrizeAbility(PrizeAbility::None,[&](size_t,bool)->double{++touches;return 1;},[&](size_t,double,bool){++touches;});assert(touches==0);
 // Framing is verified by the all-angle, four-resolution bounds regression.
 PrizePlan plan;plan.id=561;plan.enablePaintjob=true;plan.serial=123;FakeBackend backend;PrizeLoadGate gate;
 auto report=probePrize(plan,backend);assert(report.stage==PrizeStage::Model&&backend.requested.empty()&&gate.shouldFlush(plan,report)&&!gate.shouldFlush(plan,report));
 backend.carLoaded=true;report=probePrize(plan,backend);assert(report.stage==PrizeStage::Upgrades&&report.total==11&&backend.requested.size()==11&&backend.txdRequests==1&&gate.shouldFlush(plan,report)&&!gate.shouldFlush(plan,report));
 // Every upgrade can be streaming-loaded without querying its render-object
 // pointer. modelReady asserts that it is only ever called for the vehicle.
 backend.loaded=backend.requested;report=probePrize(plan,backend);assert(report.stage==PrizeStage::Paint&&report.loaded==11&&gate.shouldFlush(plan,report)&&!gate.shouldFlush(plan,report));backend.paintLoaded=true;report=probePrize(plan,backend);assert(report.ready()&&!gate.shouldFlush(plan,report));
 backend.invalid=*backend.loaded.begin();report=probePrize(plan,backend);assert(report.stage==PrizeStage::Invalid&&!gate.shouldFlush(plan,report));backend.invalid=-1;
 plan.modkit=false;plan.enablePaintjob=false;backend.car=485;plan.id=485;backend.requested.clear();backend.remapReads=backend.txdRequests=0;backend.paintLoaded=false;report=probePrize(plan,backend);assert(report.ready()&&backend.requested.empty()&&backend.remapReads==0&&backend.txdRequests==0);backend.carLoaded=false;report=probePrize(plan,backend);assert(gate.shouldFlush(plan,report));++plan.serial;assert(gate.shouldFlush(plan,report));
 std::array<uint8_t,0x100> vehicle{};int16_t id=485;std::memcpy(vehicle.data()+0x22,&id,2);assert(prizeNative::tune(vehicle.data(),plan));
 const auto path=std::filesystem::temp_directory_path()/(L"gt4-secret-"+std::to_wstring(GetCurrentProcessId())+L".ini");assert(!std::filesystem::exists(path));auto parsed=readSecretUnlocks(path.wstring());Entry calica;calica.id=15183;calica.modelKey=modelNameKey("calica");assert(parsed.matches(calica)&&!parsed.matches(baggage)&&!parsed.matches(stratum)&&!parsed.matches(addon)&&parsed.enableModkit&&parsed.enableAbilities&&!parsed.enablePaintjob);
 assert(WritePrivateProfileStringW(L"SecretUnlocks",L"Model",L"20042, sultan",path.c_str()));assert(WritePrivateProfileStringW(L"SecretUnlocks",L"EnableModkit",L"0",path.c_str()));assert(WritePrivateProfileStringW(L"SecretUnlocks",L"EnableAbilities",L"1",path.c_str()));parsed=readSecretUnlocks(path.wstring());assert(parsed.matches(addon)&&parsed.matches(sultan)&&!parsed.matches(stratum)&&!parsed.enableModkit&&parsed.enableAbilities);
 assert(WritePrivateProfileStringW(L"SecretUnlocks",L"Model",L"",path.c_str()));parsed=readSecretUnlocks(path.wstring());assert(parsed.models.ids.empty()&&parsed.models.names.empty());assert(WritePrivateProfileStringW(L"SecretUnlocks",L"Model",L"-1,@@@,485",path.c_str()));parsed=readSecretUnlocks(path.wstring());assert(parsed.models.invalid==2&&parsed.matches(baggage));std::filesystem::remove(path);
 std::puts("SecretUnlocks: mixed IDs/names/add-ons/empty list, actual Win32 reload/defaults, optional compatible modkits, 10,000 single abilities and disabled no-op, native load-state probe vs render pointers, relocated TXD stage, bounded paused completion passes and ordinary bonus vehicles passed.");
}
