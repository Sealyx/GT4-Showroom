#include <windows.h>
#include "MinHook.h"
#ifdef PREVIEW_PEER
#include "../source/NativePreview.hpp"
#endif
using Render=void(__thiscall*)(void*);Render next=nullptr;void* address=nullptr;unsigned calls=0;
void __fastcall callback(void*self,void*){++calls;
#ifdef PREVIEW_PEER
 if(gt4::nativePreview::onEntityRender(self))return;
#endif
 next(self);
}
extern "C" __declspec(dllexport) int __stdcall Start(void* entry){address=entry;calls=0;return MH_Initialize()==MH_OK&&MH_CreateHook(entry,reinterpret_cast<void*>(callback),reinterpret_cast<void**>(&next))==MH_OK&&MH_EnableHook(entry)==MH_OK;}
extern "C" __declspec(dllexport) unsigned __stdcall Count(){return calls;}
extern "C" __declspec(dllexport) void __stdcall Target(void* value){
#ifdef PREVIEW_PEER
 gt4::nativePreview::target=value;
#else
 (void)value;
#endif
}
extern "C" __declspec(dllexport) int __stdcall Stop(){return MH_DisableHook(address)==MH_OK&&MH_RemoveHook(address)==MH_OK&&MH_Uninitialize()==MH_OK;}
