#pragma once
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>

namespace qb {
inline bool readableCode(uintptr_t address,size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if(!address || !VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))) return false;
    if(info.State!=MEM_COMMIT || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return false;
    const DWORD access=info.Protect&0xFF;
    if(access!=PAGE_EXECUTE_READ && access!=PAGE_EXECUTE_READWRITE && access!=PAGE_EXECUTE_WRITECOPY) return false;
    const uintptr_t base=reinterpret_cast<uintptr_t>(info.BaseAddress);
    return address>=base && address-base<info.RegionSize && size<=info.RegionSize-(address-base);
}
inline bool validateEntryJump(uintptr_t entry,uintptr_t& destination) {
    std::array<uintptr_t,8> seen{};
    uintptr_t current=entry;
    for(size_t i=0;i<seen.size();++i) {
        for(size_t j=0;j<i;++j) if(seen[j]==current) return false;
        seen[i]=current;
        if(!readableCode(current,1)) return false;
        const uint8_t opcode=*reinterpret_cast<const uint8_t*>(current);
        if(opcode!=0xE9) {
            if(!i || opcode==0xEB || opcode==0xCC) return false;
            destination=current; return true;
        }
        if(!readableCode(current,5)) return false;
        int32_t displacement;
        std::memcpy(&displacement,reinterpret_cast<const void*>(current+1),sizeof(displacement));
        const uintptr_t next=static_cast<uint32_t>(current+5+static_cast<uint32_t>(displacement));
        for(size_t j=0;j<=i;++j) if(next>=seen[j] && next-seen[j]<5) return false;
        current=next;
    }
    return false;
}
}
