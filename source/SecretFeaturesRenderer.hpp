#pragma once
#include "Renderer.hpp"
#include "SecretFeaturesState.hpp"
#include "ResultButtonArt.hpp"
namespace gt4 {
class SecretFeaturesRenderer {
 IDirect3DTexture9* scene=nullptr;UINT width=0,height=0;D3DFORMAT sceneFormat=D3DFMT_UNKNOWN;
 static void button(IDirect3DDevice9*d,Renderer&r,const FailRect&b,bool selected,float alpha){
  const auto&profile=selected?resultButtonGold:resultButtonGray;const float radius=std::min(b.h*.5f,b.w*.5f);
  // Flat scanline colors reproduce the GT4 bevel without state-dependent
  // Gouraud interpolation. The rounded silhouette remains procedural.
  for(size_t y=0;y<profile.size();++y){const float t=(float(y)+.5f)/float(profile.size()),dy=b.h*(t-.5f);const float inset=radius-std::sqrt(std::max(0.f,radius*radius-dy*dy));const float top=b.y+b.h*float(y)/float(profile.size()),bottom=b.y+b.h*float(y+1)/float(profile.size());r.rect(d,b.x+inset,top,b.w-2*inset,bottom-top,color(profile[y],alpha));}
 }

public:
 ~SecretFeaturesRenderer(){reset();}SecretFeaturesRenderer()=default;SecretFeaturesRenderer(const SecretFeaturesRenderer&)=delete;SecretFeaturesRenderer&operator=(const SecretFeaturesRenderer&)=delete;
 bool ready()const{return true;}
 void reset(){releaseScene();}
 void releaseScene(){if(scene)scene->Release();scene=nullptr;width=height=0;sceneFormat=D3DFMT_UNKNOWN;}
 bool draw(IDirect3DDevice9*d,Renderer&r,const SecretFeaturesState&s){
  if(!d||!s.active)return false;r.ensureUiAssets(d);if(!r.uiTextReady())return false;D3DVIEWPORT9 vp{};d->GetViewport(&vp);IDirect3DSurface9*current=nullptr;if(FAILED(d->GetRenderTarget(0,&current)))return false;D3DSURFACE_DESC desc{};current->GetDesc(&desc);
  IDirect3DTexture9*shown=nullptr;
  if(!shown){if(!scene||width!=desc.Width||height!=desc.Height||sceneFormat!=desc.Format){releaseScene();if(FAILED(d->CreateTexture(desc.Width,desc.Height,1,D3DUSAGE_RENDERTARGET,desc.Format,D3DPOOL_DEFAULT,&scene,nullptr))){current->Release();return false;}width=desc.Width;height=desc.Height;sceneFormat=desc.Format;}IDirect3DSurface9*copy=nullptr;if(FAILED(scene->GetSurfaceLevel(0,&copy))){current->Release();return false;}const HRESULT copied=d->StretchRect(current,nullptr,copy,nullptr,D3DTEXF_NONE);copy->Release();current->Release();if(FAILED(copied))return false;shown=scene;}else current->Release();
  IDirect3DStateBlock9*saved=nullptr;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&saved)))return false;saved->Capture();
  d->SetPixelShader(nullptr);d->SetVertexShader(nullptr);d->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_GOURAUD);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);d->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);d->SetRenderState(D3DRS_DITHERENABLE,FALSE);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);d->SetRenderState(D3DRS_CLIPPLANEENABLE,0);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);d->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_CURRENT);d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
  const float w=float(vp.Width),h=float(vp.Height);const SecretFeaturesLayout l(w,h,s.age);const auto words=failureText(s.kind);r.rect(d,0,0,w,h,0xFF000000);r.rect(d,l.scene.x,l.scene.y,l.scene.w,l.scene.h,0xFFFFFFFF,shown);
  const float maxWidth=std::min(w*.34f,w-l.word.x-w*.035f);
  r.label(d,words.label,l.label.x,l.label.y,r.fitted(words.label,l.label.h,maxWidth,true),l.labelAlpha,false,0,true);
  r.text(d,words.word,l.word.x,l.word.y,r.fitted(words.word,l.word.h,maxWidth,true),color(0xFFFFFF,l.wordAlpha),false,0,true);
  if(s.interactive()){for(int n=0;n<2;++n){const auto&b=l.buttons[n];button(d,r,b,n==s.selected,l.buttonAlpha);r.text(d,n?"EXIT":"RETRY",b.x+b.w*.5f,b.y+b.h*.22f,r.fitted(n?"EXIT":"RETRY",l.buttonTextHeight,b.w*.82f,true),color(0x101010,l.buttonAlpha),true,0,true);}if(l.buttonAlpha>.01f)r.cursor(d,s.pointerX,s.pointerY,h/720.f,l.buttonAlpha);}
  saved->Apply();saved->Release();return true;
 }
};
}
