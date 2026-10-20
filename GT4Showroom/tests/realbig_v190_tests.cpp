#define GTA_BINDING_TEST
#include "../source/NativePrize.hpp"
#include "../source/PrizeFraming.hpp"
#include <cassert>
#include <map>
#include <set>
#include <filesystem>
using namespace gt4;
struct FakeAbilities {
 struct Car{int generation=1,id=15183;uintptr_t handling=0;float mass=1500,turn=3500,buoy=14000;uint32_t flags=0x200;};
 std::map<uintptr_t,Car> cars;int reads=0,writes=0;
 int reference(uintptr_t v){++reads;return int(v)*256+cars.at(v).generation;}
 bool valid(const AbilityState& s){++reads;auto it=cars.find(s.pointer);return it!=cars.end()&&it->second.id==s.model&&s.reference==int(s.pointer)*256+it->second.generation;}
 void baseline(AbilityState&s){const auto& c=cars.at(s.pointer);s.mass=c.mass;s.turnMass=c.turn;s.buoyancy=c.buoy;s.addedFlags=(s.ability==PrizeAbility::Proof?0x840000u:1u)&~c.flags;}
 void enforce(AbilityState&s){++writes;auto& c=cars.at(s.pointer);c.flags|=s.ability==PrizeAbility::Proof?0x840000u:1u;if(s.ability==PrizeAbility::HeavyRamming){c.mass=s.targetMass;c.turn=s.targetTurnMass;c.buoy=s.targetBuoyancy;}}
 void restore(const AbilityState&s){++writes;auto& c=cars.at(s.pointer);c.flags&=~s.addedFlags;if(s.ability==PrizeAbility::HeavyRamming){c.mass=s.mass;c.turn=s.turnMass;c.buoy=s.buoyancy;}}
};
static std::map<uintptr_t,std::vector<uint8_t>> fake;
template<class T>void put(uintptr_t address,const T& value){fake[address]=std::vector<uint8_t>(sizeof(T));std::memcpy(fake[address].data(),&value,sizeof(T));}
static bool readFake(uintptr_t address,void* out,size_t count){for(const auto& row:fake)if(address>=row.first&&address-row.first+count<=row.second.size()){std::memcpy(out,row.second.data()+address-row.first,count);return true;}return gta::processRead(address,out,count);}
static uint8_t __fastcall vehicleType(void*,void*){return 6;}
int main(){
 assert(prizeStreamingFlags==8);
 std::vector<TuningOption> candidates={{15846,44},{15847,44},{15848,44},{15849,45},{15850,45},{15851,14},{15852,14},{15853,51},{15854,51},{15855,51},{15856,6},{15857,6},{15858,6},{15859,8},{15860,8},{1010,15,true},{1086,17,true},{1087,16,true},{1073,34},{1074,34}};
 std::map<int,int> slot;for(const auto&o:candidates)slot[o.id]=o.slot;std::set<int> choices;
 for(int i=0;i<1000;++i){const auto parts=chooseTuning(candidates,uint32_t(i),i%3);assert(parts.size()==10);std::set<int> selected;for(int id:parts){assert(slot.count(id)&&selected.insert(slot[id]).second);choices.insert(id);}}
 assert(choices.size()==candidates.size());assert(chooseTuning({},1,0).empty());assert(tuningSlot(0)==-1&&tuningSlot(0x8000|0x100|(12<<10))==44);
 // Actual runtime discovery traverses the live registered model's carmods
 // fields. There is no stock-ID whitelist and no file-name guessing here.
 std::vector<void*> models(20010);std::array<uint8_t,0x400> vehicle{};std::vector<std::array<uint8_t,0x20>> upgrades(candidates.size());std::array<uintptr_t,5> vtable{};vtable[4]=uintptr_t(&vehicleType);auto* vt=vtable.data();std::memcpy(vehicle.data(),&vt,sizeof(vt));void* rw=reinterpret_cast<void*>(1);std::memcpy(vehicle.data()+0x1C,&rw,4);models[15183]=vehicle.data();
 for(int i=0;i<18;++i){const int16_t empty=-1;std::memcpy(vehicle.data()+0x2D6+i*2,&empty,2);}
 for(size_t i=0;i<candidates.size();++i){const auto& o=candidates[i];const bool dummy=o.slot>=32;const uint16_t flags=uint16_t(0x8000|(dummy?0x100:0)|((dummy?o.slot-32:o.slot)<<10));std::memcpy(upgrades[i].data()+0x12,&flags,2);models[size_t(o.id)]=upgrades[i].data();if(i<18){const int16_t id=int16_t(o.id);std::memcpy(vehicle.data()+0x2D6+i*2,&id,2);}}
 put<uint32_t>(0x40CD58,20010);put<uint32_t>(0x40CD67,uint32_t(uintptr_t(models.data())));gta::reader=readFake;
 PrizePlan plan;plan.id=15183;plan.style=2;plan.serial=987;plan.enablePaintjob=false;assert(prizeNative::configure(plan));assert(plan.kitResolved&&plan.paintResolved&&plan.upgrades.size()==9&&plan.remapTxd==-1);for(int id:plan.upgrades)assert(id>=1000);
 put<uintptr_t>(0xC8800C,0x200000);put<uintptr_t>(0x200004,0x300000);put<int>(0x200008,100);put<uint8_t>(0x300000+42,0);int adds=0,removes=0,requests=0;
 auto add=[&](int id){assert(id==42);++adds;};auto request=[&](int id,int flags){assert(id==42&&flags==8);++requests;};auto remove=[&](int id){assert(id==42);++removes;};
 assert(prizeNative::queueTxdWith(42,add,request));assert(prizeNative::queueTxdWith(42,add,request));assert(adds==1&&requests==2);prizeNative::releaseTxdsWith(remove);prizeNative::releaseTxdsWith(remove);assert(removes==1&&prizeNative::heldTxds.empty());put<uint8_t>(0x300000+42,0x80);assert(!prizeNative::queueTxdWith(42,add,request)&&adds==1&&requests==2);
 gta::reader=gta::processRead;
 // Actual Win32 defaults and independent switches, including paint with kit off.
 const auto ini=std::filesystem::temp_directory_path()/(L"gt4-v190-"+std::to_wstring(GetCurrentProcessId())+L".ini");assert(!std::filesystem::exists(ini));auto config=readSecretUnlocks(ini.wstring());Entry calica;calica.id=15183;calica.modelKey=modelNameKey("calica");assert(config.matches(calica)&&config.enableModkit&&!config.enablePaintjob&&config.enableAbilities);
 WritePrivateProfileStringW(L"SecretUnlocks",L"EnableModkit",L"0",ini.c_str());WritePrivateProfileStringW(L"SecretUnlocks",L"EnablePaintjob",L"1",ini.c_str());WritePrivateProfileStringW(L"SecretUnlocks",L"EnableAbilities",L"0",ini.c_str());config=readSecretUnlocks(ini.wstring());assert(!config.enableModkit&&config.enablePaintjob&&!config.enableAbilities);std::filesystem::remove(ini);
 // Pointer + generation + model identity: a recycled pool slot cannot inherit
 // an old reward. Repeated maintenance uses baseline targets, not multiplication.
 FakeAbilities backend;backend.cars[1].handling=123;backend.cars[2].handling=123;PrizeAbilities<FakeAbilities> abilities;
 assert(abilities.add(1,15183,PrizeAbility::HeavyRamming,backend));assert(backend.cars[1].mass==9000&&backend.cars[1].turn==8750&&backend.cars[1].buoy==84000);assert(backend.cars[1].handling==123&&backend.cars[2].handling==123);
 for(int i=0;i<1000;++i){backend.cars[1].mass=1500;abilities.maintain(backend);assert(backend.cars[1].mass==9000);}
 assert(abilities.add(2,15183,PrizeAbility::Proof,backend));backend.cars[2].flags=0x200;abilities.maintain(backend);assert(backend.cars[2].flags==(0x200|0x840000));assert(backend.cars[2].mass==1500);const int reads=backend.reads,writes=backend.writes;assert(abilities.add(3,15183,PrizeAbility::None,backend));assert(reads==backend.reads&&writes==backend.writes);
 backend.cars[2].generation++;backend.cars[2].flags=0x400;abilities.maintain(backend);assert(backend.cars[2].flags==0x400&&abilities.size()==1);
 abilities.clear(backend);assert(backend.cars[1].handling==123&&backend.cars[1].mass==1500&&backend.cars[1].flags==0x200);
 for(int i=10;i<45;++i){backend.cars[uintptr_t(i)].handling=123;assert(abilities.add(uintptr_t(i),15183,PrizeAbility::HeavyRamming,backend));}assert(abilities.size()==32&&backend.cars[10].handling==123);abilities.clear(backend);
 // Settled framing remains below the raised title/name safe region at
 // all rotations, including tall compact vehicles and very long add-ons.
 for(auto wh:std::vector<std::pair<float,float>>{{800,600},{1280,720},{1920,1080},{2560,1080}})for(auto shape:std::vector<V3>{{2,5,1.6f},{1,2,3},{2,12,3},{3,3,2},{.7f,2,1.1f}})for(int angle=0;angle<360;angle+=6)for(float time:{6.7f,7.2f,9.f,19.f}){
  const auto pose=prizePose(time);const auto cam=prizeCamera(wh.first,wh.second,pose);const float size=prizeMeshScale(std::max({shape.x,shape.y,shape.z}),shape.z);const float a=angle*pi/180;
  for(int i=0;i<8;++i){const float x=(i&1?.5f:-.5f)*shape.x*size,y=(i&2?.5f:-.5f)*shape.y*size;const V3 point{x*std::cos(a)-y*std::sin(a),x*std::sin(a)+y*std::cos(a),(i&4?shape.z:0)*size};float px,py,z,rhw;assert(cam.project(point,px,py,z,rhw));assert(px>wh.first*.04f&&px<wh.first*.96f);assert(py>wh.second*(prizeNameY+.06f)&&py<wh.second*.86f);}
 }
 std::puts("REALBIG1.9: actual live add-on carmods discovery; one compatible option per slot; independent kit/paint/ability INI; zero-flag TXD discard regression; preserved plugin-owned handling and maintained physical proof/mass, pool recycling/restore/32-car bound; all-angle four-resolution framing passed. No GTA session.");
}
