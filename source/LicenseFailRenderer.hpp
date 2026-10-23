#pragma once
#include "Renderer.hpp"
#include "LicenseFail.hpp"
namespace gt4 {
// GPU-only rolling camera recorder. No ReadRenderTargetData or CPU frame copies.
// Default capacity90at15Hz, at most384x216pixels/frame (~28.5MiB RGBA).
class LicenseReplay {
 std::array<IDirect3DTexture9*,90> frames{};size_t head=0,count=0;uint64_t serial=0,next=0,last=0;bool frozen=false;
 UINT w=0,h=0,sourceW=0,sourceH=0;D3DFORMAT format=D3DFMT_UNKNOWN;IDirect3DTexture9*resolve=nullptr;
 bool copy(IDirect3DDevice9*d,IDirect3DSurface9*from,const D3DSURFACE_DESC&desc,IDirect3DSurface9*to){
  if(desc.MultiSampleType==D3DMULTISAMPLE_NONE)return SUCCEEDED(d->StretchRect(from,nullptr,to,nullptr,D3DTEXF_LINEAR));
  // D3D9 multisample resolution must keep dimensions; scale in a second blit.
  if(!resolve&&FAILED(d->CreateTexture(desc.Width,desc.Height,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&resolve,nullptr)))return false;
  IDirect3DSurface9*flat=nullptr;if(FAILED(resolve->GetSurfaceLevel(0,&flat)))return false;const bool ok=SUCCEEDED(d->StretchRect(from,nullptr,flat,nullptr,D3DTEXF_NONE))&&SUCCEEDED(d->StretchRect(flat,nullptr,to,nullptr,D3DTEXF_LINEAR));flat->Release();return ok;
 }
public:
 ~LicenseReplay(){reset();}LicenseReplay()=default;LicenseReplay(const LicenseReplay&)=delete;LicenseReplay&operator=(const LicenseReplay&)=delete;
 void reset(){for(auto*&f:frames){if(f)f->Release();f=nullptr;}if(resolve)resolve->Release();resolve=nullptr;head=count=0;serial=next=last=0;w=h=sourceW=sourceH=0;format=D3DFMT_UNKNOWN;frozen=false;}
 size_t size()const{return count;}size_t bytes()const{return size_t(w)*h*4*count;}bool isFrozen()const{return frozen;}
 bool capture(IDirect3DDevice9*d,uint64_t now,uint64_t generation,bool eligible,bool keep){
  if(!eligible){if(!keep&&last&&now-last>700)reset();return false;}if(!d)return false;
  if(generation!=serial){reset();serial=generation;}if(frozen||now<next)return false;
  IDirect3DSurface9*from=nullptr;if(FAILED(d->GetRenderTarget(0,&from)))return false;D3DSURFACE_DESC desc{};from->GetDesc(&desc);
  if(sourceW&& (sourceW!=desc.Width||sourceH!=desc.Height||format!=desc.Format)){reset();serial=generation;}
  if(desc.Format!=D3DFMT_X8R8G8B8&&desc.Format!=D3DFMT_A8R8G8B8){from->Release();return false;}sourceW=desc.Width;sourceH=desc.Height;format=desc.Format;const float aspect=float(desc.Width)/std::max(1u,desc.Height);w=384;h=UINT(std::max(1.f,std::round(384.f/aspect)));if(h>216){h=216;w=UINT(std::max(1.f,std::round(216.f*aspect)));}
  if(!frames[head]&&FAILED(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,format,D3DPOOL_DEFAULT,&frames[head],nullptr))){from->Release();return false;}
  IDirect3DSurface9*to=nullptr;if(FAILED(frames[head]->GetSurfaceLevel(0,&to))){from->Release();return false;}const bool ok=copy(d,from,desc,to);to->Release();from->Release();if(!ok)return false;
  head=(head+1)%frames.size();count=std::min(count+1,frames.size());last=now;next=now+66;return true;
 }
 IDirect3DTexture9*image(double age){if(!count)return nullptr;frozen=true;const size_t oldest=(head+frames.size()-count)%frames.size();const size_t offset=size_t(std::max(0.,age)*15.)%count;return frames[(oldest+offset)%frames.size()];}
};
class LicenseFailRenderer {
 IDirect3DTexture9* replay=nullptr;UINT width=0,height=0;D3DFORMAT replayFormat=D3DFMT_UNKNOWN;
 static void button(IDirect3DDevice9*d,const FailRect&b,bool selected,float alpha){
  const float radius=std::min(b.h*.5f,b.w*.5f);const uint32_t rgb=selected?0xD69E47:0xB0B3B6;
  auto vertex=[&](float x,float y){return ScreenVertex{x-.5f,y-.5f,0,1,color(rgb,alpha),0,0};};
  std::vector<ScreenVertex>v;v.push_back(vertex(b.x+b.w*.5f,b.y+b.h*.5f));
  for(int corner=0;corner<4;++corner)for(int n=0;n<=8;++n){const float a=-pi*.5f+float(corner)*pi*.5f+float(n)*pi/16.f;const float cx=corner<2?b.x+b.w-radius:b.x+radius,cy=corner==0||corner==3?b.y+radius:b.y+b.h-radius;v.push_back(vertex(cx+std::cos(a)*radius,cy+std::sin(a)*radius));}v.push_back(v[1]);
  d->SetFVF(format);d->SetTexture(0,nullptr);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);d->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,UINT(v.size()-2),v.data(),sizeof(ScreenVertex));
 }
public:
 LicenseReplay recorder;
 ~LicenseFailRenderer(){reset();}LicenseFailRenderer()=default;LicenseFailRenderer(const LicenseFailRenderer&)=delete;LicenseFailRenderer&operator=(const LicenseFailRenderer&)=delete;
 bool ready()const{return true;}
 void reset(){releaseReplay();recorder.reset();}
 void releaseReplay(){if(replay)replay->Release();replay=nullptr;width=height=0;replayFormat=D3DFMT_UNKNOWN;}
 bool draw(IDirect3DDevice9*d,Renderer&r,const LicenseFailState&s){
  if(!d||!s.active)return false;r.ensureUiAssets(d);if(!r.uiTextReady())return false;D3DVIEWPORT9 vp{};d->GetViewport(&vp);IDirect3DSurface9*current=nullptr;if(FAILED(d->GetRenderTarget(0,&current)))return false;D3DSURFACE_DESC desc{};current->GetDesc(&desc);
  IDirect3DTexture9*shown=recorder.image(s.age);
  if(!shown){if(!replay||width!=desc.Width||height!=desc.Height||replayFormat!=desc.Format){releaseReplay();if(FAILED(d->CreateTexture(desc.Width,desc.Height,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&replay,nullptr))){current->Release();return false;}width=desc.Width;height=desc.Height;replayFormat=desc.Format;}IDirect3DSurface9*copy=nullptr;if(FAILED(replay->GetSurfaceLevel(0,&copy))){current->Release();return false;}const HRESULT copied=d->StretchRect(current,nullptr,copy,nullptr,D3DTEXF_NONE);copy->Release();current->Release();if(FAILED(copied))return false;shown=replay;}else current->Release();
  IDirect3DStateBlock9*saved=nullptr;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&saved)))return false;saved->Capture();
  d->SetPixelShader(nullptr);d->SetVertexShader(nullptr);d->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_DITHERENABLE,FALSE);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_CLIPPLANEENABLE,0);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_CURRENT);d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
  const float w=float(vp.Width),h=float(vp.Height);const LicenseFailLayout l(w,h,s.age);const auto words=failureText(s.kind);r.rect(d,0,0,w,h,0xFF000000);r.rect(d,l.replay.x,l.replay.y,l.replay.w,l.replay.h,0xFFFFFFFF,shown);
  const float maxWidth=std::min(w*.34f,w-l.word.x-w*.035f);
  r.label(d,words.label,l.label.x,l.label.y,r.fitted(words.label,l.label.h,maxWidth,true),l.labelAlpha,false,0,true);
  r.text(d,words.word,l.word.x,l.word.y,r.fitted(words.word,l.word.h,maxWidth,true),color(0xFFFFFF,l.wordAlpha),false,0,true);
  if(s.interactive()){for(int n=0;n<2;++n){const auto&b=l.buttons[n];button(d,b,n==s.selected,l.buttonAlpha);r.text(d,n?"EXIT":"RETRY",b.x+b.w*.5f,b.y+b.h*.04f,r.fitted(n?"EXIT":"RETRY",l.buttonTextHeight,b.w*.82f,true),color(0x101010,l.buttonAlpha),true,0,true);}if(l.buttonAlpha>.01f)r.cursor(d,s.pointerX,s.pointerY,h/720.f,l.buttonAlpha);}
  saved->Apply();saved->Release();return true;
 }
};
}
