#include "../source/Showroom.hpp"
#define GTA_BINDING_TEST
#include "../source/Handling.hpp"
#include "../source/RuntimeColors.hpp"
#include "../source/ModelFilter.hpp"
#include "../source/InputState.hpp"
#include "../source/KeyboardInput.hpp"
#include <cassert>
#include <map>
#include <cstring>
#include <cstdio>
using namespace gt4;
std::map<uintptr_t,std::vector<uint8_t>> memory;
template<class T>void put(uintptr_t a,T x){std::vector<uint8_t>b(sizeof(T));std::memcpy(b.data(),&x,sizeof(T));memory[a]=b;}
bool fixture(uintptr_t a,void*out,size_t n){auto it=memory.find(a);if(it==memory.end()||it->second.size()!=n)return false;std::memcpy(out,it->second.data(),n);return true;}
int main(){std::vector<Entry> cars(250);for(int i=0;i<250;++i){cars[i].id=20000+i;cars[i].paints.resize(3);}Model m;m.begin(0);assert(!m.open);m.begin(cars.size());assert(m.open&&m.carAlpha()==0&&m.cue==Cue::Open);m.step(.1);assert(m.carAlpha()==0);for(int i=0;i<20;++i)m.step(.1);assert(m.carAlpha()==1);m.move(-1,cars);assert(m.selected==249&&m.carAlpha()==0);for(int i=0;i<120;++i)m.step(1./60);assert(m.scroll>238&&m.scroll<=239.01f);m.accept(cars);assert(m.page==Page::Paint&&m.cue==Cue::Accept);m.move(-1,cars);assert(m.paint==2);m.favorite(cars);assert(cars[249].favorite);m.accept(cars);assert(m.spawn&&m.cue==Cue::Spawn);m.recovered();m.back();assert(m.open&&m.page==Page::Cars);m.back();assert(m.open&&m.fade.action==CloseAction::Exit);m.step(.3);m.presented();assert(m.fade.ready(m.clock));m.open=false;m.begin(cars.size());const auto clock=m.clock;m.step(-10);assert(m.clock==clock);m.step(.5);assert(std::abs(m.clock-clock-.5)<.000001);m.step(NAN);m.step(INFINITY);assert(std::abs(m.clock-clock-.5)<.000001);const auto capped=m.clock;m.step(1000);assert(m.clock-capped==1);for(int i=0;i<10000;++i){m.move(i%2?1:-1,cars);m.step(.01);assert(m.selected<cars.size()&&m.carAlpha()>=0&&m.carAlpha()<=1&&m.scroll>=0&&m.scroll<=240);}
 gta::reader=fixture;put<uint16_t>(0x6B0B9C,0xC069);put<uint16_t>(0x6B0BA2,0x808D);put<uint32_t>(0x6B0B9E,0x120);put<uint32_t>(0x6B0BA4,0x30000000);const uint16_t id=599;const uintptr_t p=0x30000000+id*0x120;put<uint32_t>(p,id);put<float>(p+4,1400);put<char>(p+0x74,'4');put<char>(p+0x75,'P');put<uint8_t>(p+0x76,5);put<float>(p+0x7C,.003f);put<float>(p+0x84,1.f);auto h=handling(id);assert(h.valid&&h.mass==1400&&std::abs(h.limit-180)<.01f&&std::abs(h.acceleration-30)<.01f);put<float>(p+4,NAN);assert(!handling(id).valid);put<float>(p+4,1400);put<uint32_t>(p,600);assert(!handling(id).valid);put<uint32_t>(p,id);put<uint32_t>(0x6B0B9E,0x100000);assert(!handling(id).valid);
 put<uint32_t>(0x40CD67,0x32000000);put<uint32_t>(0x40CD58,25000);void* info=reinterpret_cast<void*>(0x34000000);put<void*>(0x32000000+20042*4,info);assert(gta::modelInfo(20042)==info);assert(!gta::modelInfo(25000)&&!gta::modelInfo(-1));put<uint32_t>(0x40CD67,0x33000000);assert(!gta::modelInfo(20042));put<void*>(0x33000000+20042*4,info);assert(gta::modelInfo(20042)==info);

 // A relocated 135-colour palette must not read the zero-filled vanilla table.
 put<uint8_t>(0x447094,0);memory[0x447094]={0x8D,0x04,0x85};memory[0x6A6FF7]={0x8B,0x0C,0x85};
 put<uint32_t>(0x447097,0x35000000);put<uint32_t>(0x6A6FFA,0x35000000);
 put<uint32_t>(0xB4E480+4*3,0);put<uint32_t>(0x35000000+4*3,0xFFAA6622);put<uint32_t>(0x35000000+4*134,0xFF1F7BC4);
 put<uint32_t>(0x35000000,0xFF000000);uint32_t rgb=0x999999;
 assert(gta::vehicleColor(3,rgb)&&rgb==0x2266AA);assert(gta::vehicleColor(134,rgb)&&rgb==0xC47B1F);assert(gta::vehicleColor(0,rgb)&&rgb==0); // legitimate black stays black
 assert(!gta::vehicleColor(135,rgb));put<uint32_t>(0x6A6FFA,0x36000000);assert(!gta::vehiclePalette());
 put<uint32_t>(0x447097,0xB4E480);put<uint32_t>(0x6A6FFA,0xB4E480);put<uint32_t>(0xB4E480+4*3,0xFFAA6622);assert(gta::vehicleColor(3,rgb)&&rgb==0x2266AA);assert(!gta::vehicleColor(128,rgb));
 memory[0x447094]={0xE9,0,0};assert(!gta::vehiclePalette());
 // Defaults exclude only the 49 explicitly listed vanilla IDs. Add-on IDs survive.
 std::set<int> hidden;for(auto preset:filterDefaults)assert(parseModelIds(preset,hidden)==0);assert(hidden.size()==49&&hidden.count(435)&&hidden.count(539)&&hidden.count(595)&&!hidden.count(400)&&!hidden.count(20042));
 assert(parseModelIds("20042, 400,400,-1,999999999999999999999,401junk,1-9",hidden)==4);assert(hidden.count(20042)&&hidden.count(400)&&!hidden.count(401));hidden.clear();assert(parseModelIds(" , \t,",hidden)==0&&hidden.empty());
 std::vector<Entry> many(588);for(size_t i=0;i<many.size();++i){many[i].id=int(i)+400;many[i].paints.resize(8);}
 for(const auto resolution:std::vector<std::pair<float,float>>{{1280,720},{1916,1080},{1440,810},{2560,1080},{800,600}}){
  for(size_t index:{size_t(0),size_t(1),size_t(224),size_t(582),size_t(587)}){
   Model ui;ui.selected=index;ui.begin(many.size());const Carousel strip(resolution.first,resolution.second,ui,many.size());assert(strip.last-strip.first<=20);assert(index>=size_t(strip.first)&&index<size_t(strip.last));const auto chosen=strip.box(int(index));assert(chosen.x>=0&&chosen.x+chosen.w<=resolution.first);
   if(index+1<many.size()){const auto next=strip.box(int(index+1));assert(int(index+1)<strip.last&&next.x+next.w<=resolution.first);ui.pointerX=(next.x+next.w*.5f)/resolution.first;ui.pointerY=(next.y+next.h*.5f)/resolution.second;pointerClick(ui,many,resolution.first,resolution.second);assert(ui.selected==index+1);}
   const Carousel now(resolution.first,resolution.second,ui,many.size());const auto selected=now.box(int(ui.selected));ui.pointerX=(selected.x+selected.w*.5f)/resolution.first;ui.pointerY=(selected.y+selected.h*.5f)/resolution.second;pointerClick(ui,many,resolution.first,resolution.second);assert(ui.page==Page::Paint&&!ui.spawn);
   const auto swatch=paintBox(resolution.first,resolution.second,6,8);ui.pointerX=(swatch.x+swatch.w*.5f)/resolution.first;ui.pointerY=(swatch.y+swatch.h*.5f)/resolution.second;pointerClick(ui,many,resolution.first,resolution.second);assert(ui.paint==6&&!ui.spawn);
   // The former SPAWN button region is inert, including its old hit area.
   ui.pointerX=.5f+170*(resolution.second/720.f)/resolution.first;ui.pointerY=.78f;pointerClick(ui,many,resolution.first,resolution.second);assert(!ui.spawn&&ui.page==Page::Paint);
   ui.back();assert(ui.open&&ui.page==Page::Cars&&!ui.spawn);ui.back();assert(ui.open&&!ui.spawn&&ui.fade.action==CloseAction::Exit);
   ui.begin(many.size());ui.back();assert(ui.open&&ui.fade.action==CloseAction::Exit); // Cars -> Backspace -> exit, without visiting paints.
  }
 }
 Model pointer;pointer.pointerMove(100000,-100000,1280,720);assert(pointer.pointerX==1&&pointer.pointerY==0);pointer.pointerMove(NAN,0,1280,720);assert(pointer.pointerX==1);

 std::array<int16_t,24> merged{},physical{};merged[0x22/2]=128;ControllerInput controller;controller.capture(physical.data());assert(!controller.down(0x22));physical[0x20/2]=128;controller.capture(physical.data());assert(controller.down(0x20)&&!controller.down(0x22));
 ExitButtons handoff;merged[0]=75;handoff.arm(merged.data());handoff.filter(merged.data());assert(merged[0]==75&&merged[0x22/2]==0);merged[0x22/2]=128;handoff.filter(merged.data());assert(merged[0x22/2]==0);handoff.filter(merged.data());assert(handoff.mask==0);merged[0x22/2]=128;handoff.filter(merged.data());assert(merged[0x22/2]==128);
 Key latch;assert(latch.poll(true,100));assert(!latch.poll(true,101));latch.blockUntilRelease();assert(!latch.poll(true,99999,true));assert(!latch.poll(false,100000));assert(latch.poll(true,100001));
 OpenIntent intent;intent.request(100);assert(intent.pending(2099)&&!intent.pending(2101));intent.clear();assert(!intent.pending(100));
 bool paused=false;int starts=0,ends=0;auto start=[&]{paused=true;++starts;};auto end=[&]{paused=false;++ends;};PauseOwnership lease;
 lease.begin(paused,start);assert(lease.owned&&paused&&starts==1);lease.maintain(paused,false,false,start);assert(starts==1);
 // Native pause enters without closing the model; native Resume clears UserPause.
 Model resume;resume.begin(many.size());resume.selected=224;resume.page=Page::Paint;resume.paint=6;const double savedTime=resume.clock;
 lease.maintain(paused,true,false,start);paused=false;lease.maintain(paused,false,false,start);assert(paused&&starts==2&&resume.open&&resume.selected==224&&resume.paint==6&&resume.page==Page::Paint&&resume.clock==savedTime);
 lease.end(false,end);assert(!lease.owned&&!paused&&ends==1);
 paused=true;lease.begin(paused,start);lease.end(false,end);assert(paused&&ends==1); // external pause not ours
 paused=false;lease.begin(paused,start);lease.end(true,end);assert(paused&&!lease.owned&&ends==1); // reset while menu owns pause
 // Native F1..F12 live in FKeys, not standardKeys[VK_F1].
 std::array<uint8_t,0x270> keyboard{};auto native=[&](size_t o,int16_t value){std::memcpy(keyboard.data()+o,&value,2);};
 native(0x18+0x70*2,128);assert(!nativeKeyDown(keyboard.data(),0x70));native(0,128);assert(nativeKeyDown(keyboard.data(),0x70)&&!nativeKeyDown(keyboard.data(),0x71));
 native(22,128);assert(nativeKeyDown(keyboard.data(),0x7B));native(0x18+32*2,128);assert(nativeKeyDown(keyboard.data(),32));
 native(0x254,128);native(0x25A,128);native(0x264,128);assert(nativeKeyDown(keyboard.data(),8)&&nativeKeyDown(keyboard.data(),13)&&nativeKeyDown(keyboard.data(),17));
 assert(!nativeKeyDown(nullptr,0x70)&&!nativeKeyDown(keyboard.data(),-1)&&!nativeKeyDown(keyboard.data(),256)&&!nativeKeyDown(keyboard.data(),1));
 // Every confirmation key uses one latch. Holding it cannot cross both pages.
 for(int confirmKey:{13,32}){keyboard.fill(0);Model ui;ui.begin(many.size());Key confirm;const size_t offset=confirmKey==13?0x23C:0x18+32*2;auto down=[&]{return nativeKeyDown(keyboard.data(),13)||nativeKeyDown(keyboard.data(),32);};native(offset,128);assert(confirm.poll(down(),0));ui.accept(many);assert(ui.page==Page::Paint&&!ui.spawn);assert(!confirm.poll(down(),500));native(offset,0);assert(!confirm.poll(down(),501));native(offset,128);assert(confirm.poll(down(),502));ui.accept(many);assert(ui.spawn);ui.back();assert(ui.open&&ui.spawn&&ui.page==Page::Paint&&ui.busy());ui.recovered();ui.back();assert(ui.page==Page::Cars);}
 FunctionKeyClaim claimed;keyboard.fill(0);native(0,128);native(0x18+71*2,128);claimed.filter(keyboard.data(),0x70,false,true);assert(nativeKeyDown(keyboard.data(),0x70));claimed.filter(keyboard.data(),0x70,true,true);assert(!nativeKeyDown(keyboard.data(),0x70)&&nativeKeyDown(keyboard.data(),71));native(0,128);claimed.filter(keyboard.data(),0x70,false,true);assert(!nativeKeyDown(keyboard.data(),0x70));claimed.filter(keyboard.data(),0x70,false,false);native(0,128);claimed.filter(keyboard.data(),0x70,false,true);assert(nativeKeyDown(keyboard.data(),0x70));
 OpenGate gate;assert(gate.blocked()&&!gate.ready());gate.gameState=9;gate.focus=gate.alive=true;assert(gate.ready());
 gate.menu=true;assert(gate.blocked());Key blockedOpen;assert(blockedOpen.poll(true,0)); // recorded but rejected
 assert(!blockedOpen.poll(false,1));gate.menu=false;assert(blockedOpen.poll(true,2)&&gate.ready()); // no extra rearming press
 gate.loadingData=true;assert(gate.blocked());gate.loadingData=false;gate.codePause=true;assert(gate.blocked());gate.codePause=false;gate.userPause=true;assert(!gate.blocked()&&!gate.ready());gate.userPause=false;gate.enex=2;assert(!gate.ready());gate.enex=0;gate.cutscene=true;assert(!gate.ready());gate.cutscene=false;gate.focus=false;assert(gate.blocked());gate.focus=true;gate.inVehicle=true;assert(gate.blocked()&&!gate.ready());gate.inVehicle=false;assert(gate.ready());
 std::puts("F-key native layout, both Backspace paths, no Spawn click target, confirmation latches, opening gates; palette relocation/135 colours, legitimate black, 49 default exclusions, add-on IDs, 588-car layouts/mouse hits, pause lease, input quarantine and existing logic fixtures passed.");}
