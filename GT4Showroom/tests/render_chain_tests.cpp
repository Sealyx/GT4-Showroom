#include <windows.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
struct Peer{
 HMODULE module;int(__stdcall*start)(void*);int(__stdcall*stop)();unsigned(__stdcall*count)();void(__stdcall*target)(void*);
 explicit Peer(const std::filesystem::path& p){module=LoadLibraryW(p.c_str());assert(module);start=reinterpret_cast<decltype(start)>(GetProcAddress(module,"Start"));stop=reinterpret_cast<decltype(stop)>(GetProcAddress(module,"Stop"));count=reinterpret_cast<decltype(count)>(GetProcAddress(module,"Count"));target=reinterpret_cast<decltype(target)>(GetProcAddress(module,"Target"));assert(start&&stop&&count&&target);}
 ~Peer(){FreeLibrary(module);}
};
int main(int argc,char**argv){assert(argc==2);const std::filesystem::path directory=argv[1];Peer pass(directory/"render-pass.dll"),preview(directory/"render-preview.dll");
 // An isolated x86 thiscall native body: NOP prologue, ++self->counter, RET.
 const uint8_t bytes[]={0x90,0x90,0x90,0x90,0x90,0xFF,0x01,0xC3};void*entry=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);assert(entry);std::memcpy(entry,bytes,sizeof(bytes));using Render=void(__thiscall*)(void*);auto draw=reinterpret_cast<Render>(entry);
 for(int order=0;order<2;++order){Peer&first=order?preview:pass;Peer&second=order?pass:preview;assert(first.start(entry)&&second.start(entry));std::array<uint32_t,16> world{},privatePreview{};draw(world.data());assert(world[0]==1&&pass.count()==1&&preview.count()==1);
  preview.target(privatePreview.data());draw(privatePreview.data());assert(privatePreview[0]==0&&preview.count()==2&&pass.count()==unsigned(order?2:1));
  preview.target(nullptr);draw(world.data());assert(world[0]==2&&preview.count()==3&&pass.count()==unsigned(order?3:2));assert(second.stop()&&first.stop());assert(std::memcmp(entry,bytes,sizeof(bytes))==0);
 }
 VirtualFree(entry,0,MEM_RELEASE);std::puts("Separate-DLL MinHook render chains passed in both load orders: normal draws exactly once, private target suppressed, pass-through peer retained, original bytes restored. Mirrors the paused Quantum Break forwarding path; no GTA run.");
}
