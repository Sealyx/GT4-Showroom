#pragma once
#include <array>
#include <cstdint>
namespace gt4 {
using DriverCtor=void(__thiscall*)(void*,void*,void*);
using DriverProcess=bool(__thiscall*)(void*,void*);
inline bool runDriverTask(void* ped,void* vehicle,DriverCtor construct,DriverProcess process){
 alignas(4) std::array<uint8_t,0x1C> task{};construct(task.data(),vehicle,nullptr);task[0x18]=1;
 const bool finished=process(task.data(),ped);auto** table=*reinterpret_cast<void***>(task.data());
 reinterpret_cast<void*(__thiscall*)(void*,unsigned)>(table[0])(task.data(),0);return finished;
}
}
