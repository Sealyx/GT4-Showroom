#pragma once
// Controlled host for the actual StudioControls.inl input/state implementation.
// No GTA functions/hooks: only native mouse/keyboard values and model services are supplied.
#include "../source/Renderer.hpp"
#include "../source/StudioConfig.hpp"
#include "../source/Controller.hpp"
#include "../source/SearchInput.hpp"
#include <set>
#include <cassert>
namespace gt4 {
inline Model state;inline Renderer renderer;inline StudioConfig studioConfig;inline std::vector<Entry>entries,catalog;inline std::map<int,std::unique_ptr<Mesh>> hostMeshes;
inline std::wstring base;inline SearchInput searchInput;inline float mouseSensitivity=1;inline std::map<int,uint64_t>pending;
inline Key left,right,up,down,enter,backspace,back,favorite,addFavorite,mouseLeft,mouseRight,searchKey;inline std::array<bool,256>hostKeys{};
inline std::array<uint8_t,40>hostMouse{};inline std::set<int>hostExcluded;
struct HostAudio{void endPrize(bool){}};inline HostAudio audio;
inline bool key(int vk){return hostKeys[size_t(vk)];}
namespace game {template<class T>T& at(uintptr_t address){assert(address>=0xB73418&&address+sizeof(T)<=0xB73418+hostMouse.size());return *reinterpret_cast<T*>(hostMouse.data()+address-0xB73418);}inline bool flag(uintptr_t address){assert(address==0xBA6744||address==0xBA6745);return false;}}
inline const Mesh*lookup(int id){auto it=hostMeshes.find(id);return it==hostMeshes.end()?nullptr:it->second.get();}
inline void blockActions(){searchInput.quarantine();for(Key*k:{&left,&right,&up,&down,&enter,&backspace,&back,&favorite,&addFavorite,&mouseLeft,&mouseRight,&searchKey})k->blockUntilRelease();state.dragging=false;}
inline void refreshSelected(){}
inline unsigned hostReloads=0;inline void reloadAll(){++hostReloads;}
inline void refreshCountryMenu(){std::array<int,countryCount>counts{};for(const auto&e:catalog)if(!hostExcluded.count(e.id)&&(!state.ui.favoritesOnly||e.favorite))++counts[size_t(e.country)];state.ui.menu.setCountries(counts);}
inline void rebuildCatalogueView(){buildCatalogueView(state,entries,catalog,studioConfig.sort,[](const Entry&e){return hostExcluded.count(e.id)!=0;},[](const Entry&e){return e.favorite;});pending.clear();refreshCountryMenu();}
inline void reloadExclusions(){for(auto&e:catalog)e.favorite=GetPrivateProfileIntW(L"Favorites",std::to_wstring(e.id).c_str(),0,(base+L"GT4Showroom\\favorites.ini").c_str())!=0;rebuildCatalogueView();}
#include "../source/StudioControls.inl"
inline void hostFrame(const PadActions&p={},double dt=1./60,float w=1280,float h=720){state.step(dt);interaction(p,w,h,dt,uint64_t(state.clock*1000));}
inline void settleView(){for(int n=0;n<90;++n){hostFrame();state.presented();}assert(!state.busy());}
inline void hostRelease(){hostKeys.fill(false);hostMouse.fill(0);hostFrame();}
inline void hostKey(int vk){hostRelease();hostKeys[size_t(vk)]=true;hostFrame();hostRelease();}
}
