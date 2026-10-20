#define GTA_BINDING_TEST
#include "../source/FrontendState.hpp"
#include "../source/KeyboardInput.hpp"
#include <array>
#include <cassert>
#include <cstdio>
namespace {
// An independent memory image of CMenuManager, including the old false
// 'loading' address in the mouse-X field. Constants come from native code/SDK.
constexpr uintptr_t imageBase=0xBA6748;
std::array<uint8_t,0x100> memory{};bool unreadable=false;
bool readImage(uintptr_t p,void*out,size_t n){
 if(unreadable||p<imageBase||p-imageBase+n>memory.size())return false;
 std::memcpy(out,memory.data()+p-imageBase,n);return true;
}
gt4::OpenGate gate(){const auto flags=gt4::readFrontendState();gt4::OpenGate g;g.gameState=9;g.alive=g.focus=true;g.menu=flags.menuActive();g.loadingData=flags.loadingData();return g;}
}
int main(){
 gta::reader=readImage;
 for(uint8_t mouseX:{192,201,209})for(uint8_t shown:{0,1}){
  memory.fill(0);memory[0xB8]=shown;memory[0xBC]=mouseX;
  const auto flags=gt4::readFrontendState();assert(flags.valid&&!flags.menuActive()&&!flags.loadingData()&&gate().ready());
  uint8_t oldLoading=0;assert(readImage(0xBA67A4+0x60,&oldLoading,1)&&oldLoading==mouseX); // reproduced bad binding
 }
 // The actual native menu flag must block even when the old mouse fields say 0.
 memory.fill(0);memory[0x5C]=1;assert(gt4::readFrontendState().menuActive()&&gate().blocked());
 memory[0x5C]=0;memory[0x60]=1;assert(gt4::readFrontendState().loadingData()&&gate().blocked());
 memory[0x60]=0;memory[0x5D]=1;assert(gt4::readFrontendState().startLoadRequested()&&gate().ready());
 // Native byte semantics: nonzero is true, never an invalid C++ bool object.
 for(uint8_t value:{uint8_t(2),uint8_t(192),uint8_t(209),uint8_t(255)}){
  memory[0x60]=value;const auto flags=gt4::readFrontendState();assert(flags.loadingData());assert(int(flags.loadingData())==1&&gate().blocked());
 }
 unreadable=true;assert(!gt4::readFrontendState().valid&&gate().blocked());
 std::puts("Frontend address regression passed: mouse-X bytes 192/201/209 cannot block opening; actual menu/loading flags block; request separate; nonzero flags normalized; unreadable memory fails closed.");
}
