#pragma once
#include <array>
#include <d3d9.h>

namespace gt4 {
template<class Device=IDirect3DDevice9,class Surface=IDirect3DSurface9>class RenderTargets {
 Device*device=nullptr;std::array<Surface*,4>targets{};Surface*depth=nullptr;D3DVIEWPORT9 viewport{};RECT scissor{};DWORD count=0;bool captured=false,changed=false;
public:
 explicit RenderTargets(Device*d):device(d){
  if(!device)return;D3DCAPS9 caps{};if(FAILED(device->GetDeviceCaps(&caps))||!caps.NumSimultaneousRTs||caps.NumSimultaneousRTs>targets.size())return;
  count=caps.NumSimultaneousRTs;
  for(DWORD i=0;i<count;++i){const HRESULT hr=device->GetRenderTarget(i,&targets[i]);if((i==0&&(!targets[i]||FAILED(hr)))||(i>0&&FAILED(hr)&&hr!=D3DERR_NOTFOUND))return;}
  const HRESULT hr=device->GetDepthStencilSurface(&depth);if((FAILED(hr)&&hr!=D3DERR_NOTFOUND)||FAILED(device->GetViewport(&viewport))||FAILED(device->GetScissorRect(&scissor)))return;captured=true;
 }
 ~RenderTargets(){restore();for(auto*target:targets)if(target)target->Release();if(depth)depth->Release();}
 RenderTargets(const RenderTargets&)=delete;RenderTargets&operator=(const RenderTargets&)=delete;
 bool valid()const{return captured;}
 const D3DVIEWPORT9&view()const{return viewport;}
 void track(){if(captured)changed=true;}
 bool isolate(){if(!captured)return false;changed=true;for(DWORD i=1;i<count;++i)if(FAILED(device->SetRenderTarget(i,nullptr)))return false;return true;}
 bool select(Surface*target,Surface*stencil,const D3DVIEWPORT9&view){
  if(!target||!isolate())return false;
  return SUCCEEDED(device->SetDepthStencilSurface(nullptr))&&SUCCEEDED(device->SetRenderTarget(0,target))&&SUCCEEDED(device->SetDepthStencilSurface(stencil))&&SUCCEEDED(device->SetViewport(&view));
 }
 bool primary(){return captured&&select(targets[0],depth,viewport);}
 bool restore(){
  if(!captured)return false;if(!changed)return true;bool ok=SUCCEEDED(device->SetDepthStencilSurface(nullptr));
  for(DWORD i=1;i<count;++i)ok=SUCCEEDED(device->SetRenderTarget(i,nullptr))&&ok;
  ok=SUCCEEDED(device->SetRenderTarget(0,targets[0]))&&ok;
  for(DWORD i=1;i<count;++i)ok=SUCCEEDED(device->SetRenderTarget(i,targets[i]))&&ok;
  ok=SUCCEEDED(device->SetDepthStencilSurface(depth))&&ok;ok=SUCCEEDED(device->SetScissorRect(&scissor))&&ok;ok=SUCCEEDED(device->SetViewport(&viewport))&&ok;if(ok)changed=false;return ok;
 }
};
}
