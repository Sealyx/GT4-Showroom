#include "../source/Showroom.hpp"
#include "../source/ModelFilter.hpp"
#include "../source/SearchInput.hpp"
#include "../source/VehFuncsBinding.hpp"
#include "../source/WheelPose.hpp"
#include "../source/Presentation.hpp"
#include <cassert>
#include <cstdio>
using namespace gt4;
static std::vector<int> calls;static void* expected=nullptr;
void __thiscall wheelCall(void* self,int node,int flags){assert(self==expected);calls.push_back(node);assert(flags==0);}
int main(){
 assert(modelNameKey("123456789")==0x340BC6D9&&modelNameKey("peren")==modelNameKey("PEREN"));
 std::vector<Entry> e(4);e[0].id=400;e[0].name="Landstalker";e[0].modelKey=modelNameKey("landstal");e[1].id=404;e[1].name="Perennial";e[1].modelKey=modelNameKey("peren");e[2].id=554;e[2].name="Yosemite";e[2].modelKey=modelNameKey("yosemite");e[3].id=20042;e[3].name="Custom Family Van";e[3].modelKey=modelNameKey("addvan");
 assert(findModel(e,"404")==1&&findModel(e,"PeReN")==1&&findModel(e,"perennial")==1&&findModel(e,"perenni")==1&&findModel(e,"family")==3&&findModel(e,"addvan")==3&&findModel(e," 20042 ")==3);
 for(const auto* bad:{"","999999999999999999999","404junk","4044","not present"})assert(findModel(e,bad)==e.size());
 ExclusionConfig filter;parseModelTokens("400,peren,Custom Family Van,PEREN,20042,-1,1000000,@bad",filter);assert(filter.ids.size()==2&&filter.names.size()==2&&filter.invalid==3&&filter.hides(e[0])&&filter.hides(e[1])&&filter.hides(e[3])&&!filter.hides(e[2]));
 std::vector<Entry> visible;for(const auto&entry:e)if(!filter.hides(entry))visible.push_back(entry);assert(findModel(visible,"peren")==visible.size());
 Model ui;ui.begin(e.size());ui.page=Page::Paint;ui.spawn=true;ui.beginSearch();assert(ui.searching&&ui.page==Page::Cars&&!ui.spawn);ui.query="peren";ui.submitSearch(e);assert(!ui.searching&&ui.selected==1&&!ui.spawn&&ui.page==Page::Cars&&ui.searchNotice.find("MODEL FOUND")==0);ui.beginSearch();ui.query="bad";ui.submitSearch(e);assert(ui.searching&&ui.searchFailed&&!ui.spawn&&ui.selected==1);
 SearchInput input;std::array<bool,256> down{};auto poll=[&](uint64_t tick){return input.poll([&](int key){return down[size_t(key)];},tick,ui.query);};ui.query.clear();input.quarantine();down['F']=true;poll(0);assert(ui.query.empty());down['F']=false;poll(1);down['F']=true;poll(2);poll(900);assert(ui.query=="F");down['F']=false;poll(901);down[8]=true;poll(902);assert(ui.query.empty());assert(poll(1500)==SearchAction::Cancel);down[8]=false;poll(1501);down[13]=true;assert(poll(1502)==SearchAction::Submit);assert(poll(2000)==SearchAction::None);down[13]=false;poll(2001);ui.query=std::string(48,'A');down['B']=true;poll(2002);assert(ui.query.size()==48);
 assert(sideSpawnDistance(-1,1)==2&&sideSpawnDistance(-1.4f,1.2f)>2.39f&&sideSpawnDistance(0,0)==2.6f);
 const uint8_t code[]={0x55,0x8B,0xEC,0x8B,0x4D,0x08,0x03,0x0D,0,0,0,0,0x8A,0x41,0x09,0x24,0xFE,0x0A,0x45,0x0C,0x88,0x41,0x09,0x5D,0xC3};assert(hideExportLayout(code,sizeof(code)));auto changed=std::array<uint8_t,25>{};std::memcpy(changed.data(),code,25);changed[14]=10;assert(!hideExportLayout(changed.data(),25)&&!hideExportLayout(code,24));
 std::array<uint8_t,0xD0> frame{};VehFuncsFrames vf;vf.verified=true;vf.offset=0xB0;assert(vf.visible(frame.data()));frame[0xB9]=1;assert(!vf.visible(frame.data()));frame[0xB9]=2;assert(vf.visible(frame.data()));vf.verified=false;frame[0xB9]=1;assert(vf.visible(frame.data()));
 expected=frame.data();assert(prepareWheelPose(expected,0,554,[](int n){return n!=3&&n!=6;},wheelCall));assert((calls==std::vector<int>{2,5,4,7}));calls.clear();assert(!prepareWheelPose(expected,9,522,[](int){return true;},wheelCall)&&calls.empty());
 std::puts("Search/name+ID exclusions: CRC/native-name/display matches, no hidden-model bypass, query editing/held keys, found/missing flow, 48-char cap; close spawn width and attached VehFuncs visibility signature/frame bits/native wheel-helper ABI passed.");
}
