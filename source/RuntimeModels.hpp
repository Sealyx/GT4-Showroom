#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstddef>
namespace gta {
using Reader=bool(*)(uintptr_t,void*,size_t);
inline bool processRead(uintptr_t address,void* output,size_t size) {
    MEMORY_BASIC_INFORMATION info{};
    if(!address || !size || !VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))) return false;
    if(info.State!=MEM_COMMIT || (info.Protect&(PAGE_NOACCESS|PAGE_GUARD))) return false;
    const DWORD access=info.Protect&255;
    if(access==PAGE_EXECUTE) return false;
    const uintptr_t base=reinterpret_cast<uintptr_t>(info.BaseAddress);
    if(address<base || address-base>=info.RegionSize || size>info.RegionSize-(address-base)) return false;
    std::memcpy(output,reinterpret_cast<const void*>(address),size); return true;
}
#ifdef GTA_BINDING_TEST
inline Reader reader=processRead;
#else
constexpr Reader reader=processRead;
#endif
template<class T> bool read(uintptr_t address,T& result) { return reader(address,&result,sizeof(result)); }
struct ModelRegistry { uintptr_t table=0; uint32_t count=0; };
inline ModelRegistry registry() {
    ModelRegistry result;
    uint32_t address=0;
    if(!read(0x40CD67,address) || !read(0x40CD58,result.count)) return {};
    if(!address || result.count<1 || result.count>1000000) return {};
    result.table=address; return result;
}
inline void* modelInfo(int model) {
    const auto models=registry();
    if(model<0 || static_cast<uint32_t>(model)>=models.count) return nullptr;
    void* info=nullptr;
    return read(models.table+static_cast<uintptr_t>(model)*sizeof(void*),info)?info:nullptr;
}
inline bool modelReady(int model) {
    void* info=modelInfo(model); void* object=nullptr;
    return info && read(reinterpret_cast<uintptr_t>(info)+0x1C,object) && object;
}
inline bool pedInfo(void* info) {
    uintptr_t vtable=0,method=0;
    if(!info || !read(reinterpret_cast<uintptr_t>(info),vtable) || !read(vtable+4*sizeof(void*),method)) return false;
    MEMORY_BASIC_INFORMATION page{};
    if(!VirtualQuery(reinterpret_cast<void*>(method),&page,sizeof(page)) || page.State!=MEM_COMMIT ||
       (page.Protect&(PAGE_NOACCESS|PAGE_GUARD)) || !(page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))) return false;
    return reinterpret_cast<uint8_t(__thiscall*)(void*)>(method)(info)==7;
}
inline int pedType(int model) {
    void* info=modelInfo(model); uint32_t type=UINT32_MAX;
    return pedInfo(info) && read(reinterpret_cast<uintptr_t>(info)+0x28,type)?static_cast<int>(type):-1;
}
}
