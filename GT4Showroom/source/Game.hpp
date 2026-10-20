#pragma once
#include "Renderer.hpp"
#include "RuntimeModels.hpp"
#include "RuntimeColors.hpp"
#include "PaintNames.hpp"
#include "InputState.hpp"
#include "KeyboardInput.hpp"
#include "FrontendState.hpp"
#include "DriverSeat.hpp"
#include "Handling.hpp"
#include "HookEntry.hpp"
#include <memory>
namespace gt4::game {
template<class T>T& at(uintptr_t p){return *reinterpret_cast<T*>(p);}
template<class T>T& field(void*p,size_t o){return *reinterpret_cast<T*>(static_cast<uint8_t*>(p)+o);}
struct Matrix{V3 right;uint32_t a;V3 up;uint32_t b;V3 forward;uint32_t c;V3 pos;uint32_t d;};
inline V3 point(const Matrix&m,V3 p){return m.pos+m.right*p.x+m.up*p.y+m.forward*p.z;}
inline bool supported(){auto* b=reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));if(uintptr_t(b)!=0x400000)return false;auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;auto* pe=reinterpret_cast<IMAGE_NT_HEADERS*>(b+dos->e_lfanew);return pe->Signature==IMAGE_NT_SIGNATURE&&pe->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&pe->OptionalHeader.SizeOfImage>=0x900000&&(at<uint32_t>(0x401000)==0x53EC8B55||at<uint32_t>(0x401000)==0x16197BE9);}
inline bool vehicleInfo(void*p){uintptr_t vt=0,fn=0;if(!p||!gta::read(uintptr_t(p),vt)||!gta::read(vt+16,fn)||!qb::readableCode(fn,1))return false;return reinterpret_cast<uint8_t(__thiscall*)(void*)>(fn)(p)==6;}
inline void* player(){return reinterpret_cast<void*(__cdecl*)(int)>(0x56E210)(-1);}
inline void* pad(){return reinterpret_cast<void*(__cdecl*)(int)>(0x53FB70)(0);}
inline bool mission(){return reinterpret_cast<bool(__cdecl*)()>(0x464D50)();}
inline V3 position(void*p){void*m=field<void*>(p,0x14);return m?field<V3>(m,0x30):field<V3>(p,4);}
inline bool flag(uintptr_t address){return at<uint8_t>(address)!=0;}
inline bool userPaused(){return flag(0xB7CB49);}
inline bool codePaused(){return flag(0xB7CB48);}
inline bool menu(){return readFrontendState().menuActive();}
// +0x5D is a request to start loading, not the active loading flag.
// Keep it in diagnostics; gate on current game state and actual loading data.
inline bool loading(){return at<int>(0xC8D4C0)!=9||readFrontendState().loadingData();}
struct OpenState:OpenGate{FrontendState frontend;};
inline OpenState openState(bool focus){OpenState g;g.gameState=at<int>(0xC8D4C0);g.frontend=readFrontendState();g.loadingData=g.frontend.loadingData();g.menu=g.frontend.menuActive();g.userPause=userPaused();g.codePause=codePaused();g.focus=focus;g.cutscene=flag(0xB5F851)||flag(0xB5F852);g.enex=at<int>(0x96A7CC);if(g.gameState==9&&!g.loadingData){void*p=player();g.alive=p&&field<float>(p,0x540)>0;g.inVehicle=p&&(field<uint32_t>(p,0x46C)&0x100)!=0;}return g;}
inline IDirect3DDevice9* device(){return reinterpret_cast<IDirect3DDevice9*(__cdecl*)()>(0x7F9D50)();}
inline std::vector<Paint> paints(void*mi){const int n=std::min(8,int(field<uint8_t>(mi,0x2D0)));std::vector<Paint> out;for(int i=0;i<std::max(n,1);++i){Paint p;for(int j=0;j<4;++j){p.ids[j]=n?field<uint8_t>(mi,0x2B0+j*8+i):uint8_t(1);gta::vehicleColor(p.ids[j],p.rgb[j]);}p.name=paintNames.name(p.ids[0],p.rgb[0]);if(p.rgb[1]!=p.rgb[0])p.name+=" / "+paintNames.name(p.ids[1],p.rgb[1]);out.push_back(p);}return out;}
inline Entry entry(int id,void*mi){Entry e;e.id=id;e.modelKey=field<uint32_t>(mi,4);e.kind=field<int>(mi,0x3C);e.classId=field<int8_t>(mi,0x4D);e.category=classLabel(e.classId);char key[9]{};std::memcpy(key,static_cast<uint8_t*>(mi)+0x32,8);const char*t=*key?reinterpret_cast<const char*(__thiscall*)(void*,const char*)>(0x6A0050)(reinterpret_cast<void*>(0xC1B340),key):nullptr;char buffer[128]{};size_t i=0;if(t)for(;i<100&&t[i];++i)buffer[i]=uint8_t(t[i])<128?t[i]:'?';e.name=*buffer&&std::strcmp(buffer,"NULL")?buffer:(*key?std::string(key):"Vehicle "+std::to_string(id));e.paints=paints(mi);const auto h=handling(field<uint16_t>(mi,0x4A));e.topSpeed=h.valid?h.limit:-1;e.information=stats(h);return e;}
inline int previewSeed(int id){return int((uint32_t(id)*1664525u+1013904223u)&0x7FFFFFFF);}
inline void seedVehicle(void* vehicle,int id){if(auto module=GetModuleHandleW(L"VehFuncs.asi")){using Seed=void(__cdecl*)(void*,int);if(auto set=reinterpret_cast<Seed>(GetProcAddress(module,"Ext_SetCarSeed")))set(vehicle,previewSeed(id));}}
inline void queue(int id){reinterpret_cast<void(__cdecl*)(int,int)>(0x4087E0)(id,0);}
// CStreaming::Update exits while paused; progress its normal loading channels once.
inline void pumpStreaming(){reinterpret_cast<void(__cdecl*)()>(0x40E3A0)();}
inline void request(int id){queue(id);reinterpret_cast<void(__cdecl*)(bool)>(0x40EA10)(false);}
struct Extract{Mesh* out;size_t count=0;bool(*visible)(void*)=nullptr;};
inline void* __cdecl extractAtomic(void*atomic,void*data){
 auto& c=*static_cast<Extract*>(data);if(!(field<uint8_t>(atomic,2)&4)||c.count>=120000)return atomic;
 void*g=field<void*>(atomic,0x18),*f=field<void*>(atomic,4);
 if(!g||!f||(c.visible&&!c.visible(f))||(field<uint32_t>(g,8)&0x01000000))return atomic;
 const char*node=reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(f);
 if(!previewHighDetail(node,uintptr_t(field<void*>(atomic,0x48))))return atomic;
 auto* mat=static_cast<Matrix*>(reinterpret_cast<void*(__cdecl*)(void*)>(0x7F0990)(f));if(!mat)return atomic;
 const int nv=field<int>(g,0x14),nm=field<int>(g,0x24);auto**materials=field<void**>(g,0x20);
 void*morph=field<void*>(g,0x5C);auto*pos=morph?field<V3*>(morph,0x14):nullptr;
 auto*normals=morph?field<V3*>(morph,0x18):nullptr;struct UV{float u,v;};auto*uv=field<UV*>(g,0x34);
 if(!pos||!materials||nv<3||nv>500000||nm<1||nm>1024)return atomic;
 auto capture=[&](void*material,auto enumerate){
  if(!material||c.count>=120000)return;
  // Only retain materials owned by this geometry's pinned material list.
  bool owned=false;for(int i=0;i<nm;++i)if(materials[i]==material){owned=true;break;}if(!owned)return;
  Batch b;const uint32_t rgba=field<uint32_t>(material,4);
  const uint32_t rgb=((rgba&255)<<16)|(rgba&0xFF00)|((rgba>>16)&255);b.rgb=previewMaterialColor(rgb);b.alpha=float(rgba>>24)/255.f;
  constexpr uint32_t markers[]={0x3CFF00,0xFF00AF,0x00FFFF,0xFF00FF};for(int j=0;j<4;++j)if(rgb==markers[j]){b.paint=j;b.rgb=0xFFFFFF;}
  void*tex=field<void*>(material,0);if(tex){const auto address=textureAddresses(field<uint32_t>(tex,0x50));b.addressU=address.first;b.addressV=address.second;}
  void*raster=tex?field<void*>(tex,0):nullptr;const int ext=at<int>(0xB4E9E0);
  if(raster&&ext>=int(sizeof(void*))&&ext<4096){IDirect3DTexture9*texture=field<IDirect3DTexture9*>(raster,size_t(ext));if(texture){b.texture=texture;b.texture->AddRef();}}
  enumerate([&](uint16_t ia,uint16_t ib,uint16_t ic){
   if(c.count>=120000||ia>=nv||ib>=nv||ic>=nv||ia==ib||ib==ic||ia==ic)return;
   const uint16_t ids[]={ia,ib,ic};const V3 pp[]={point(*mat,pos[ia]),point(*mat,pos[ib]),point(*mat,pos[ic])};
   for(auto p:pp)if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return;
   const V3 normal=unit(cross(pp[1]-pp[0],pp[2]-pp[0]));
   const V3 average=normals?mat->right*(normals[ia].x+normals[ib].x+normals[ic].x)+mat->up*(normals[ia].y+normals[ib].y+normals[ic].y)+mat->forward*(normals[ia].z+normals[ib].z+normals[ic].z):normal;
   const bool reverse=dot(normal,average)<0;const int order[]={0,reverse?2:1,reverse?1:2};
   for(int n=0;n<3;++n){const int k=order[n];const auto id=ids[k];b.vertices.push_back({pp[k],normals?unit(mat->right*normals[id].x+mat->up*normals[id].y+mat->forward*normals[id].z):normal,uv?uv[id].u:0,uv?uv[id].v:0});}++c.count;
  });
  if(b.vertices.empty()){if(b.texture)b.texture->Release();}else c.out->batches.push_back(std::move(b));
 };
 // RpMeshHeader/RpMesh layout verified against the classic 32-bit RW SDK.
 struct Header{uint32_t flags;uint16_t meshes,serial;uint32_t total,offset;};
 struct NativeMesh{uint16_t*indices;uint32_t count;void*material;};
 static_assert(sizeof(Header)==16&&sizeof(NativeMesh)==12,"32-bit RenderWare mesh layout");
 auto*header=field<Header*>(g,0x54);
 if(header){
  if(header->meshes>4096||header->total>2000000||header->offset>65536||header->flags>1)return atomic;
  auto*meshes=reinterpret_cast<NativeMesh*>(reinterpret_cast<uint8_t*>(header)+sizeof(Header)+header->offset);
  uint64_t total=0;for(unsigned i=0;i<header->meshes;++i){total+=meshes[i].count;if(total>header->total||!meshes[i].indices)return atomic;}
  if(total!=header->total)return atomic;
  for(unsigned i=0;i<header->meshes&&c.count<120000;++i){const auto&mesh=meshes[i];capture(mesh.material,[&](auto emit){previewTriangles(mesh.indices,mesh.count,header->flags,size_t(nv),120000-c.count,emit);});}
 }else{
  // Some runtime-built geometry has no mesh header yet; use its triangle list.
  struct Tri{uint16_t v[3],m;};const int nt=field<int>(g,0x10);auto*tris=field<Tri*>(g,0x2C);if(!tris||nt<1||nt>120000)return atomic;
  for(int mi=0;mi<nm&&c.count<120000;++mi)capture(materials[mi],[&](auto emit){for(int ti=0;ti<nt&&c.count<120000;++ti){const auto&t=tris[ti];if(t.m==mi)emit(t.v[0],t.v[1],t.v[2]);}});
 }
 return atomic;
}
inline std::unique_ptr<Mesh> snapshot(int id){void*mi=gta::modelInfo(id);if(!vehicleInfo(mi))return {};if(!gta::modelReady(id))return {};mi=gta::modelInfo(id);if(!vehicleInfo(mi))return {};reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BA0)(mi);auto**vt=field<void**>(mi,0);const uint8_t comps[2]={at<uint8_t>(0xB4E478),at<uint8_t>(0xB4E479)};void*clump=reinterpret_cast<void*(__thiscall*)(void*)>(vt[11])(mi);std::unique_ptr<Mesh> out=std::make_unique<Mesh>();if(clump){Extract ex{out.get(),0};reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,extractAtomic,&ex);reinterpret_cast<bool(__cdecl*)(void*)>(0x74A310)(clump);}at<uint8_t>(0xB4E478)=comps[0];at<uint8_t>(0xB4E479)=comps[1];reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BB0)(mi);out->compact();out->bounds();return out->empty()?nullptr:std::move(out);}
struct PauseLease:PauseOwnership{
 void begin(){PauseOwnership::begin(userPaused(),[]{reinterpret_cast<void(__cdecl*)()>(0x561AF0)();});}
 void maintain(){PauseOwnership::maintain(userPaused(),menu(),codePaused(),[]{reinterpret_cast<void(__cdecl*)()>(0x561AF0)();});}
 void end(){PauseOwnership::end(menu(),[]{reinterpret_cast<void(__cdecl*)()>(0x561B00)();});}
};
inline void* spawn(const Entry&e,size_t choice,std::string&error,float clearance=2.6f,bool inside=false){void*p=player();if(!vehicleInfo(gta::modelInfo(e.id))){error="Vehicle is no longer registered";return nullptr;}if(!p||mission()||field<uint8_t>(p,0x2F)||field<float>(p,0x540)<=0||(field<uint32_t>(p,0x46C)&0x100)||flag(0xB5F851)||flag(0xB5F852)){error="Return outdoors and leave missions before spawning";return nullptr;}if(e.kind==6){error="Rail vehicles require a train placement tool";return nullptr;}if(!gta::modelReady(e.id))request(e.id);if(!gta::modelReady(e.id)){error="Vehicle streaming failed";return nullptr;}const V3 base=position(p);const float heading=field<float>(p,0x558);V3 pos{};bool found=false;for(int r=0;r<4&&!found;++r)for(int a=0;a<8&&!found;++a){const float angle=heading+a*pi/4;pos={base.x-std::sin(angle)*(clearance+r*3),base.y+std::cos(angle)*(clearance+r*3),base.z+6};bool hit=false;float z=0;if(e.kind==5){hit=reinterpret_cast<bool(__cdecl*)(float,float,float,float*,int,int)>(0x6E8580)(pos.x,pos.y,pos.z,&z,0,0);}else z=reinterpret_cast<float(__cdecl*)(float,float,float,bool*,void**)>(0x5696C0)(pos.x,pos.y,pos.z,&hit,nullptr);if(!hit||std::abs(z-base.z)>12)continue;pos.z=z+.08f;bool blocked=false;for(int k=0;k<5;++k){V3 test=pos+V3{float(k==1?1:k==2?-1:0),float(k==3?1.6f:k==4?-1.6f:0),1.1f};if(reinterpret_cast<void*(__cdecl*)(V3,float,void*,bool,bool,bool,bool,bool,bool)>(0x569E20)(test,.8f,p,true,true,true,true,true,false))blocked=true;}if(!blocked)found=true;}
 if(!found){error=e.kind==5?"Move near clear water to spawn this boat":"No clear nearby spawn space";return nullptr;}struct Pool{void*objects;uint8_t*flags;int count;};auto*pool=at<Pool*>(0xB74494);bool free=false;if(pool&&pool->count>0&&pool->count<100000){for(int i=0;i<pool->count;++i)if(pool->flags[i]&0x80){free=true;break;}}if(!free){error="Vehicle pool is full";return nullptr;}void*v=reinterpret_cast<void*(__cdecl*)(int,V3,bool)>(0x431F80)(e.id,pos,false);if(!v){error="Vehicle pool is full";return nullptr;}seedVehicle(v,e.id);reinterpret_cast<void(__thiscall*)(void*,float)>(0x43E0C0)(v,sideOnHeading(base,pos));field<uint32_t>(v,0x4F8)=1;if(!e.paints.empty()){const auto&paint=e.paints[std::min(choice,e.paints.size()-1)];for(int j=0;j<4;++j)field<uint8_t>(v,0x434+j)=paint.ids[j];}reinterpret_cast<void(__thiscall*)(void*,int)>(0x6D5D70)(v,1);reinterpret_cast<void(__thiscall*)(void*)>(0x446F90)(v);reinterpret_cast<void(__thiscall*)(void*)>(0x532B00)(v);if(inside){void*intel=field<void*>(p,0x47C);if(intel)reinterpret_cast<void(__thiscall*)(void*,bool)>(0x601640)(intel,false);runDriverTask(p,v,reinterpret_cast<DriverCtor>(0x6470E0),reinterpret_cast<DriverProcess>(0x64B950));if(!(field<uint32_t>(p,0x46C)&0x100)||field<void*>(p,0x58C)!=v)error="Vehicle spawned, but the driver seat could not be entered.";}return v;}
}
