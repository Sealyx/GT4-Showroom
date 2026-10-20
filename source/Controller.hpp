#pragma once
#include <windows.h>
#include <Xinput.h>
#include <cmath>
#include <algorithm>
#include "InputState.hpp"
namespace gt4 {
struct PadActions{
 bool pause=false;bool connected=false,accept=false,back=false,add=false,favorites=false,up=false,down=false,left=false,right=false,view=false,search=false;
 float orbitX=0,orbitY=0,zoom=0;
};
inline float deadAxis(int v,int limit,int zone){if(std::abs(v)<=zone)return 0;return std::copysign(std::min(1.f,float(std::abs(v)-zone)/float(limit-zone)),float(v));}
inline PadActions nativeController(const ControllerInput&p){PadActions o;auto b=[&](size_t off){return p.down(off);};auto a=[&](int i){return deadAxis(p.values[size_t(i)],128,28);};o.pause=b(0x18);o.accept=b(0x20);o.back=b(0x22);o.add=b(0x1C);o.favorites=b(0x1E);o.up=b(0x10)||a(1)<-.6f;o.down=b(0x12)||a(1)>.6f;o.left=b(0x14)||a(0)<-.6f;o.right=b(0x16)||a(0)>.6f;o.view=b(0x1A);o.search=b(0x24);o.orbitX=a(2);o.orbitY=a(3);o.zoom=(b(0xE)?1.f:0.f)-(b(0xA)?1.f:0.f);if(!o.zoom)o.zoom=(b(0xC)?1.f:0.f)-(b(0x8)?1.f:0.f);o.connected=o.pause||o.accept||o.back||o.add||o.favorites||o.up||o.down||o.left||o.right||o.view||o.search||o.orbitX||o.orbitY||o.zoom;return o;}
inline PadActions xinputController(const XINPUT_GAMEPAD&p){PadActions o;o.connected=true;auto b=[&](WORD m){return (p.wButtons&m)!=0;};const float x=deadAxis(p.sThumbLX,32767,8000),y=deadAxis(p.sThumbLY,32767,8000);o.pause=b(XINPUT_GAMEPAD_START);o.accept=b(XINPUT_GAMEPAD_A);o.back=b(XINPUT_GAMEPAD_B);o.add=b(XINPUT_GAMEPAD_X);o.favorites=b(XINPUT_GAMEPAD_Y);o.up=b(XINPUT_GAMEPAD_DPAD_UP)||y>.6f;o.down=b(XINPUT_GAMEPAD_DPAD_DOWN)||y<-.6f;o.left=b(XINPUT_GAMEPAD_DPAD_LEFT)||x<-.6f;o.right=b(XINPUT_GAMEPAD_DPAD_RIGHT)||x>.6f;o.view=b(XINPUT_GAMEPAD_BACK);o.search=b(XINPUT_GAMEPAD_LEFT_THUMB);o.orbitX=deadAxis(p.sThumbRX,32767,8000);o.orbitY=-deadAxis(p.sThumbRY,32767,8000);o.zoom=(std::max(0,int(p.bRightTrigger)-30)-std::max(0,int(p.bLeftTrigger)-30))/225.f;if(std::abs(o.zoom)<.01f)o.zoom=(b(XINPUT_GAMEPAD_RIGHT_SHOULDER)?1.f:0.f)-(b(XINPUT_GAMEPAD_LEFT_SHOULDER)?1.f:0.f);return o;}
// Both sources can coexist: an idle XInput device must not hide a native pad.
inline PadActions combineControllers(PadActions a,const PadActions&b){
 a.connected=a.connected||b.connected;
 a.pause=a.pause||b.pause;a.accept=a.accept||b.accept;a.back=a.back||b.back;
 a.add=a.add||b.add;a.favorites=a.favorites||b.favorites;
 a.up=a.up||b.up;a.down=a.down||b.down;a.left=a.left||b.left;a.right=a.right||b.right;
 a.view=a.view||b.view;a.search=a.search||b.search;
 if(b.orbitX)a.orbitX=b.orbitX;if(b.orbitY)a.orbitY=b.orbitY;if(b.zoom)a.zoom=b.zoom;return a;
}
inline bool studioOpenInput(bool keyboard,bool open,const PadActions&pad){return keyboard||(!open&&pad.view);}
class Controller{
 using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);HMODULE module=nullptr;GetState get=nullptr;bool initialized=false;uint64_t retry=0;DWORD slot=0;bool connected=false;
public:
 PadActions poll(const ControllerInput&native,uint64_t tick){if(!initialized){initialized=true;for(const wchar_t*n:{L"xinput1_4.dll",L"xinput9_1_0.dll"}){module=LoadLibraryExW(n,nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(module){get=reinterpret_cast<GetState>(GetProcAddress(module,"XInputGetState"));if(get)break;FreeLibrary(module);module=nullptr;}}}if(get&&(connected||tick>=retry)){XINPUT_STATE state{};if(connected&&get(slot,&state)==ERROR_SUCCESS)return combineControllers(nativeController(native),xinputController(state.Gamepad));connected=false;for(DWORD i=0;i<4;++i)if(get(i,&state)==ERROR_SUCCESS){connected=true;slot=i;return combineControllers(nativeController(native),xinputController(state.Gamepad));}retry=tick+500;}return nativeController(native);}
 ~Controller(){if(module)FreeLibrary(module);}
};
}
