#define GTA_BINDING_TEST
#include "../source/NativePrize.hpp"
#include "../source/PrizeFraming.hpp"
#include <cassert>
#include <map>
#include <limits>
using namespace gt4;
struct Vehicle {
 alignas(16) std::array<uint8_t,0xA00> bytes{};
 std::unique_ptr<std::array<uint8_t,0x128>> pluginHandling;
 int reference=1,model=560;bool live=true;
 Vehicle(){game::field<float>(bytes.data(),0x8C)=1500;game::field<float>(bytes.data(),0x90)=3500;game::field<float>(bytes.data(),0xA0)=14000;game::field<uint32_t>(bytes.data(),0x40)=0x200;replace();}
 uintptr_t pointer(){return uintptr_t(bytes.data());}
 void replace(){pluginHandling=std::make_unique<std::array<uint8_t,0x128>>();pluginHandling->fill(0xAC);game::field<void*>(bytes.data(),0x384)=pluginHandling.get();}
 void destroy(){assert(game::field<void*>(bytes.data(),0x384)==pluginHandling.get());pluginHandling.reset();live=false;++reference;bytes.fill(0xDD);}
};
struct Backend:prizeNative::AbilityBackend {
 std::map<uintptr_t,Vehicle*> cars;int reads=0;
 int reference(uintptr_t v){++reads;return cars.at(v)->reference;}
 bool valid(const AbilityState&s){++reads;auto i=cars.find(s.pointer);return i!=cars.end()&&i->second->live&&i->second->reference==s.reference&&i->second->model==s.model;}
 void registerCar(Vehicle&v){cars[v.pointer()]=&v;}
};
int main(){
 Backend b;PrizeAbilities<Backend> a;Vehicle v,other;b.registerCar(v);b.registerCar(other);
 auto original=*v.pluginHandling;auto otherBytes=other.bytes;auto pointer=v.pluginHandling.get();
 assert(a.add(v.pointer(),560,PrizeAbility::HeavyRamming,b));
 for(int frame=0;frame<10000;++frame){
  game::field<float>(v.bytes.data(),0x8C)=1500;a.maintain(b);
  assert(game::field<float>(v.bytes.data(),0x8C)==9000);
  assert(game::field<void*>(v.bytes.data(),0x384)==pointer&&*v.pluginHandling==original&&other.bytes==otherBytes);
 }
 v.replace();pointer=v.pluginHandling.get();original=*v.pluginHandling;a.maintain(b);
 assert(game::field<void*>(v.bytes.data(),0x384)==pointer&&*v.pluginHandling==original);
 game::field<float>(v.bytes.data(),0x8C)=777;a.clear(b);
 assert(game::field<float>(v.bytes.data(),0x8C)==777&&game::field<float>(v.bytes.data(),0x90)==3500);
 assert(game::field<void*>(v.bytes.data(),0x384)==pointer&&*v.pluginHandling==original);
 game::field<float>(v.bytes.data(),0x8C)=1500;
 assert(a.add(v.pointer(),560,PrizeAbility::HeavyRamming,b));v.destroy();auto dead=v.bytes;a.maintain(b);a.clear(b);assert(a.size()==0&&v.bytes==dead);
 v.live=true;v.bytes=otherBytes;v.replace();auto recycled=v.bytes;a.maintain(b);assert(v.bytes==recycled);
 const int reads=b.reads;assert(a.add(0,0,PrizeAbility::None,b)&&b.reads==reads);
 game::field<float>(v.bytes.data(),0x8C)=std::numeric_limits<float>::quiet_NaN();auto invalid=v.bytes;
 assert(!a.add(v.pointer(),560,PrizeAbility::HeavyRamming,b)&&v.bytes==invalid);
 std::vector<std::unique_ptr<Vehicle>> many;
 for(int i=0;i<40;++i){many.push_back(std::make_unique<Vehicle>());b.registerCar(*many.back());assert(a.add(many.back()->pointer(),560,PrizeAbility::HeavyRamming,b));}
 assert(a.size()==32);for(int i=0;i<8;++i)assert(game::field<float>(many[i]->bytes.data(),0x8C)==1500);a.clear(b);
 for(auto&c:many)assert(game::field<float>(c->bytes.data(),0x8C)==1500&&game::field<void*>(c->bytes.data(),0x384)==c->pluginHandling.get());
 assert(prizeButtonWidth(1920,1080)>330&&prizeButtonHeight*1080>36&&prizeButtonTop+prizeButtonHeight<.944f);
 float zoom=7,alpha=0,cy=2;
 for(int tick=0;tick<=7200;++tick){auto pose=prizePose(double(tick)/1000);assert(std::isfinite(pose.zoom)&&pose.zoom<=zoom+.00001f&&pose.alpha>=alpha-.00001f&&pose.centerY<=cy+.00001f);zoom=pose.zoom;alpha=pose.alpha;cy=pose.centerY;}
 assert(prizePose(5.4).zoom>4&&prizePose(5.4).alpha>0&&prizePose(7.2).zoom==1&&prizePose(7.2).alpha==1);
 std::puts("Stability regression passed: 10,000 production physical-field updates; foreign extended handling replacement/destruction; pool recycling; bounded restoration; monotone reveal.");
}
