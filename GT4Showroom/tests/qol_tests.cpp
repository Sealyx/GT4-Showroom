#include "../source/Catalogue.hpp"
#include "../source/Presentation.hpp"
#include "../source/InputState.hpp"
#include "../source/DriverSeat.hpp"
#include <cassert>
#include <cstdio>
using namespace gt4;
int ctors=0,processes=0,dtors=0;void* expectedCar=reinterpret_cast<void*>(0x12345678);void*expectedPed=reinterpret_cast<void*>(0x23456789);
void* __thiscall cleanup(void*self,unsigned flags){assert(flags==0);++dtors;return self;}
void* driverVtable[]={reinterpret_cast<void*>(cleanup)};
void __thiscall ctor(void*self,void* car,void*utility){assert(car==expectedCar&&!utility);*reinterpret_cast<void***>(self)=driverVtable;++ctors;}
bool __thiscall process(void*self,void*ped){assert(ped==expectedPed&&static_cast<uint8_t*>(self)[0x18]==1);++processes;return true;}
int main(){
 std::vector<Entry> cars(588);for(size_t i=0;i<cars.size();++i){cars[i].id=int(i)+400;cars[i].paints.resize(8);}
 for(auto resolution:std::vector<std::pair<float,float>>{{1280,720},{1916,1080},{1440,810},{2560,1080},{800,600},{3840,720},{5120,1440}}){const auto[w,h]=resolution;
  for(size_t selected:{size_t(0),size_t(1),size_t(61),size_t(271),size_t(582),size_t(587)}){
   Model m;m.selected=selected;m.begin(cars.size());Carousel layout(w,h,m,cars.size());assert(layout.last-layout.first<=20);
   // Every visible partial/full cell must be in the draw/stream/hit range.
   for(int i=0;i<int(cars.size());++i){auto box=layout.box(i);if(box.x<w&&box.x+box.w>0){assert(i>=layout.first&&i<layout.last);}}
   const BrowseBar bar(w,h,m,cars.size());assert(bar.thumb.x>=bar.track.x&&bar.thumb.x+bar.thumb.w<=bar.track.x+bar.track.w+.01f);
   const FooterLayout footer(h);assert(footer.y>=h*.907f&&footer.y+footer.fontHeight*64/29<h);assert(footer.fontHeight<22*h/720);
  }
  Model m;m.begin(cars.size());BrowseBar start(w,h,m,cars.size());m.pointerX=(start.thumb.x+start.thumb.w*.5f)/w;m.pointerY=(start.track.y+start.track.h*.5f)/h;pointerButton(m,cars,w,h,true,true);assert(m.dragging);
  m.pointerX=(start.track.x+start.travel+start.thumb.w*.5f)/w;pointerButton(m,cars,w,h,true,false);assert(m.selected==587&&m.scroll==m.scrollTarget());
  pointerButton(m,cars,w,h,false,false);assert(!m.dragging);m.pointerX=(start.track.x+start.thumb.w*.5f)/w;pointerButton(m,cars,w,h,true,true);assert(m.selected==0);
  m.page=Page::Paint;pointerButton(m,cars,w,h,true,false);assert(!m.dragging&&!m.spawn);
 }
 std::vector<Entry> sorted(4);sorted[0].id=7;sorted[0].name="zebra";sorted[0].topSpeed=90;sorted[0].classId=2;sorted[1].id=4;sorted[1].name="Alpha";sorted[1].topSpeed=200;sorted[1].classId=0;sorted[2].id=5;sorted[2].name="beta";sorted[2].topSpeed=150;sorted[2].classId=1;sorted[3].id=6;sorted[3].name="alpha";sorted[3].topSpeed=-1;sorted[3].classId=0;
 sortCatalogue(sorted,{SortBy::Name,false});assert(sorted[0].id==4&&sorted[1].id==6&&sorted[3].id==7);sortCatalogue(sorted,{SortBy::Name,true});assert(sorted[0].id==7&&sorted[3].id==6);
 sortCatalogue(sorted,{SortBy::Id,false});assert(sorted.front().id==4&&sorted.back().id==7);sortCatalogue(sorted,{SortBy::Id,true});assert(sorted.front().id==7&&sorted.back().id==4);
 sortCatalogue(sorted,{SortBy::Class,false});assert(sorted.front().classId==0&&sorted.back().classId==2);sortCatalogue(sorted,{SortBy::TopSpeed,false});assert(sorted[0].id==7&&sorted[2].id==4&&sorted[3].id==6);sortCatalogue(sorted,{SortBy::TopSpeed,true});assert(sorted[0].id==4&&sorted[2].id==7&&sorted[3].id==6);
 MoveCueGate sound;assert(sound.allow(0)&&!sound.allow(33)&&!sound.allow(99)&&sound.allow(100));
 TypedReload text;uint64_t tick=100;for(char c:std::string("wrongGTSPAW"))assert(!text.feed(c,tick++));assert(text.feed('N',tick++));assert(!text.feed('N',tick++));text.feed('G',tick);assert(!text.feed('T',tick+4000));
 text={};std::array<bool,256> keys{};auto down=[&](int vk){return keys[size_t(vk)];};bool hit=false;for(char c:std::string("GTSPAWN")){keys[size_t(c)]=true;hit=text.poll(down,tick++,true);assert(!text.poll(down,tick++,true));keys[size_t(c)]=false;text.poll(down,tick++,true);}assert(hit);text.feed('G',tick);text.poll(down,tick++,false);assert(text.text.empty());text.feed('G',tick);keys['1']=true;text.poll(down,tick++,true);assert(text.text.empty());
 for(V3 position:std::vector<V3>{{0,10,0},{-4,8,0},{8,-3,0}}){const float heading=sideOnHeading({},position);V3 forward{-std::sin(heading),std::cos(heading),0};assert(std::abs(dot(position,forward))<.0001f);}
 for(int green=173;green<=175;++green)assert(previewMaterialColor(0xFF0000|uint32_t(green<<8))==0xFFFFFF);for(int green=56;green<=60;++green)assert(ivfLightMarker(0xFF0000|uint32_t(green<<8)));
 for(int red=181;red<=185;++red)assert(ivfLightMarker(uint32_t(red<<16)|0xFF00));for(int blue=198;blue<=200;++blue)assert(ivfLightMarker(0xFF00|uint32_t(blue)));for(int green=16;green<=18;++green)assert(ivfLightMarker(0xFF|uint32_t(green<<8)));
 assert(previewMaterialColor(0x01FFFF)==0xFFFFFF&&previewMaterialColor(0x123456)==0x123456&&previewMaterialColor(0)==0&&!ivfLightMarker(0xFFAC00)&&!ivfLightMarker(0xBAFF00)&&!ivfLightMarker(0x3CFF00));
 assert(runDriverTask(expectedPed,expectedCar,ctor,process));assert(ctors==1&&processes==1&&dtors==1);
 std::puts("QoL: visible edge cells/stream range at seven resolutions, bar drag to both ends, footer bounds, all sort keys/orders, GTSPAWN edges/timeouts/pause, side-on heading, IVF marker ranges and Win32 driver-task ABI passed.");
}
