#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>

namespace modding {
inline constexpr uintptr_t videoMemoryHookAddress=0x7F7F77;
using VideoMemoryCleanup=void(__cdecl*)();
extern "C" {
__attribute__((used)) inline void* moddingVideoMemoryResume=nullptr;
__attribute__((used)) inline VideoMemoryCleanup moddingVideoMemoryCleanup=nullptr;
__attribute__((used,noinline,force_align_arg_pointer)) inline void __cdecl moddingVideoMemoryDispatch() noexcept {
    try { if(moddingVideoMemoryCleanup)moddingVideoMemoryCleanup(); } catch(...) {}
}
__attribute__((used,naked)) inline void moddingVideoMemoryBridge() {
    __asm__ volatile(
        "pushfl\n\t"
        "pushal\n\t"
        "movl %esp,%eax\n\t"
        "subl $528,%esp\n\t"
        "andl $-16,%esp\n\t"
        "movl %eax,512(%esp)\n\t"
        "fxsave (%esp)\n\t"
        "cld\n\t"
        "call _moddingVideoMemoryDispatch\n\t"
        "fxrstor (%esp)\n\t"
        "movl 512(%esp),%esp\n\t"
        "popal\n\t"
        "popfl\n\t"
        "jmp *_moddingVideoMemoryResume\n\t"
    );
}
}
inline bool videoMemoryBinding(uintptr_t address=videoMemoryHookAddress) {
    constexpr uint8_t body[]={0x89,0x0D,0x40,0x24,0x8E,0x00,0x89,0x0D,0x44,0x24,0x8E,0x00};
    constexpr uint8_t bridge[]={0x9C,0x60,0x89,0xE0,0x81,0xEC,0x10,0x02,0x00,0x00,0x83,0xE4,0xF0,0x89,0x84,0x24,0x00,0x02,0x00,0x00,0x0F,0xAE,0x04,0x24,0xFC};
    std::array<uintptr_t,8> seen{};
    for(size_t i=0;i<seen.size();++i) {
        for(size_t j=0;j<i;++j)if(seen[j]==address)return false;
        seen[i]=address;
        MEMORY_BASIC_INFORMATION region{};
        if(!address||!VirtualQuery(reinterpret_cast<void*>(address),&region,sizeof(region))||region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
        const DWORD access=region.Protect&255;
        if(access!=PAGE_EXECUTE_READ&&access!=PAGE_EXECUTE_READWRITE&&access!=PAGE_EXECUTE_WRITECOPY)return false;
        const uintptr_t start=reinterpret_cast<uintptr_t>(region.BaseAddress);
        if(address<start||address-start>=region.RegionSize)return false;
        const size_t available=region.RegionSize-(address-start);
        const auto* bytes=reinterpret_cast<const uint8_t*>(address);
        if(i==0&&available>=sizeof(body)&&!std::memcmp(bytes,body,sizeof(body)))return true;
        if(i&&available>=sizeof(bridge)&&!std::memcmp(bytes,bridge,sizeof(bridge)))return true;
        if(available<5||bytes[0]!=0xE9)return false;
        int32_t offset=0;std::memcpy(&offset,bytes+1,sizeof(offset));
        address=uint32_t(address+5+offset);
    }
    return false;
}
}
