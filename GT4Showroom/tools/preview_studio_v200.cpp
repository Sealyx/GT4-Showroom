#include "../source/Renderer.hpp"
#include "../source/InputState.hpp"
#include "../source/SearchInput.hpp"
#include "../source/PaintNames.hpp"
#define GTA_BINDING_TEST
#include "../source/RuntimeColors.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <map>
#include <gdiplus.h>
using namespace gt4;
std::array<uint32_t,135> previewPalette{};
bool paletteRead(uintptr_t address,void*out,size_t size){const uint8_t lea[]={0x8D,0x04,0x85},mov[]={0x8B,0x0C,0x85};const uint32_t relocated=0x35000000;
 if(address==0x447094&&size==3){std::memcpy(out,lea,3);return true;}if(address==0x6A6FF7&&size==3){std::memcpy(out,mov,3);return true;}if((address==0x447097||address==0x6A6FFA)&&size==4){std::memcpy(out,&relocated,4);return true;}
 if(address>=relocated&&address-relocated+size<=previewPalette.size()*4){std::memcpy(out,reinterpret_cast<const uint8_t*>(previewPalette.data())+address-relocated,size);return true;}return false;
}
void paletteRoundTrip(Paint& paint){gta::reader=paletteRead;for(int k=0;k<4;++k){const uint32_t rgb=paint.rgb[k];previewPalette[128+k]=0xFF000000|((rgb&255)<<16)|(rgb&0xFF00)|((rgb>>16)&255);uint32_t resolved=0;assert(gta::vehicleColor(uint8_t(128+k),resolved)&&resolved==rgb);paint.rgb[k]=resolved;}}

template<class T>T read(std::ifstream&f){T v{};f.read(reinterpret_cast<char*>(&v),sizeof(v));assert(f);return v;}
IDirect3DTexture9* texture(IDirect3DDevice9*d,const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);assert(f);const auto w=read<uint32_t>(f),h=read<uint32_t>(f);assert(w&&h&&w*h<=16777216);IDirect3DTexture9*t=nullptr;assert(SUCCEEDED(d->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&t,nullptr)));D3DLOCKED_RECT l{};assert(SUCCEEDED(t->LockRect(0,&l,nullptr,0)));for(UINT y=0;y<h;++y)f.read(static_cast<char*>(l.pBits)+y*l.Pitch,w*4);t->UnlockRect(0);return t;}
std::unique_ptr<Mesh> load(IDirect3DDevice9*d,const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);char tag[4];f.read(tag,4);assert(!std::memcmp(tag,"GTS1",4));auto m=std::make_unique<Mesh>();std::map<std::string,IDirect3DTexture9*> textures;const auto nb=read<uint32_t>(f);assert(nb<2000);for(UINT i=0;i<nb;++i){Batch b;b.paint=read<int>(f);b.rgb=read<uint32_t>(f);b.alpha=read<float>(f);const auto n=read<uint32_t>(f),size=read<uint32_t>(f);assert(n<1000000&&size<1024);std::string name(size,'\0');f.read(name.data(),size);if(size){auto it=textures.find(name);if(it==textures.end()){b.texture=texture(d,p.parent_path()/name);textures.emplace(name,b.texture);}else{b.texture=it->second;b.texture->AddRef();}}b.vertices.resize(n);f.read(reinterpret_cast<char*>(b.vertices.data()),n*sizeof(Vertex));assert(f);m->batches.push_back(std::move(b));}
#ifndef GT4_BASELINE
 m->compact();
#endif
 m->bounds();return m;}

std::vector<uint32_t> pixels(IDirect3DDevice9*d){IDirect3DSurface9 *rt=nullptr,*cpu=nullptr;d->GetRenderTarget(0,&rt);D3DSURFACE_DESC desc{};rt->GetDesc(&desc);assert(SUCCEEDED(d->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&cpu,nullptr)));assert(SUCCEEDED(d->GetRenderTargetData(rt,cpu)));D3DLOCKED_RECT l{};assert(SUCCEEDED(cpu->LockRect(&l,nullptr,D3DLOCK_READONLY)));std::vector<uint32_t> image(desc.Width*desc.Height);for(UINT y=0;y<desc.Height;++y){const auto*row=reinterpret_cast<const uint32_t*>(static_cast<const uint8_t*>(l.pBits)+y*l.Pitch);for(UINT x=0;x<desc.Width;++x)image[y*desc.Width+x]=row[x]|0xFF000000;}cpu->UnlockRect();rt->Release();cpu->Release();return image;}
void save(IDirect3DDevice9*d,const std::filesystem::path&p){auto image=pixels(d);D3DVIEWPORT9 vp{};d->GetViewport(&vp);Gdiplus::Bitmap bitmap(vp.Width,vp.Height,vp.Width*4,PixelFormat32bppARGB,reinterpret_cast<BYTE*>(image.data()));const CLSID encoder={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};assert(bitmap.Save(p.c_str(),&encoder,nullptr)==Gdiplus::Ok);}
void poison(IDirect3DDevice9*d){d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);RECT clip={3,4,7,8};d->SetScissorRect(&clip);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);d->SetFVF(D3DFVF_XYZ);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);d->SetRenderState(D3DRS_SHADEMODE,D3DSHADE_FLAT);d->SetRenderState(D3DRS_STENCILENABLE,TRUE);d->SetRenderState(D3DRS_STENCILFUNC,D3DCMP_NEVER);d->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_TEMP);d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACENORMAL);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT2);D3DMATRIX m{};m._11=m._22=m._33=m._44=2;d->SetTransform(D3DTS_WORLD,&m);d->SetTransform(D3DTS_TEXTURE0,&m);const float plane[]={1,0,0,-100};d->SetClipPlane(0,plane);d->SetRenderState(D3DRS_CLIPPLANEENABLE,1);D3DMATERIAL9 mat{};mat.Diffuse={1,0,0,.5};d->SetMaterial(&mat);D3DLIGHT9 lamp{};lamp.Type=D3DLIGHT_DIRECTIONAL;lamp.Direction={1,0,0};lamp.Diffuse={1,0,0,1};d->SetLight(0,&lamp);d->LightEnable(0,TRUE);}
void restored(IDirect3DDevice9*d){DWORD v=0;for(const auto&pair:std::vector<std::pair<D3DRENDERSTATETYPE,DWORD>>{{D3DRS_SCISSORTESTENABLE,TRUE},{D3DRS_SRCBLEND,D3DBLEND_ONE},{D3DRS_ZFUNC,D3DCMP_ALWAYS},{D3DRS_SHADEMODE,D3DSHADE_FLAT},{D3DRS_STENCILENABLE,TRUE},{D3DRS_STENCILFUNC,D3DCMP_NEVER},{D3DRS_CLIPPLANEENABLE,1}}){d->GetRenderState(pair.first,&v);assert(v==pair.second);}d->GetFVF(&v);assert(v==D3DFVF_XYZ);d->GetTextureStageState(0,D3DTSS_RESULTARG,&v);assert(v==D3DTA_TEMP);d->GetTextureStageState(0,D3DTSS_TEXCOORDINDEX,&v);assert(v==D3DTSS_TCI_CAMERASPACENORMAL);d->GetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,&v);assert(v==D3DTTFF_COUNT2);D3DMATRIX m{};d->GetTransform(D3DTS_WORLD,&m);assert(m._11==2&&m._22==2&&m._44==2);D3DMATERIAL9 mat{};d->GetMaterial(&mat);assert(mat.Diffuse.r==1&&mat.Diffuse.g==0&&mat.Diffuse.a==.5);D3DLIGHT9 lamp{};d->GetLight(0,&lamp);assert(lamp.Direction.x==1&&lamp.Diffuse.r==1&&lamp.Diffuse.g==0);BOOL enabled=FALSE;d->GetLightEnable(0,&enabled);assert(enabled);RECT clip{};d->GetScissorRect(&clip);assert(clip.left==3&&clip.right==7);}

#include "../tests/StudioHost.hpp"
int main(int argc,char**argv){
 assert(argc==6);const std::filesystem::path shared=argv[1],camaro=argv[2],output=argv[3],font=argv[4];const int frames=std::atoi(argv[5]);std::filesystem::create_directories(output);
 ULONG_PTR gdiplus=0;Gdiplus::GdiplusStartupInput gd;assert(Gdiplus::GdiplusStartup(&gdiplus,&gd,nullptr)==Gdiplus::Ok);
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"GT4v200Preview";RegisterClassW(&wc);HWND window=CreateWindowW(wc.lpszClassName,L"Hidden shared-code showroom preview",WS_OVERLAPPEDWINDOW,0,0,1280,720,nullptr,nullptr,wc.hInstance,nullptr);assert(window);
 auto*api=Direct3DCreate9(D3D_SDK_VERSION);assert(api);D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferWidth=1280;pp.BackBufferHeight=720;pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 IDirect3DDevice9*d=nullptr;HRESULT deviceResult=api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&d);if(FAILED(deviceResult))deviceResult=api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d);if(FAILED(deviceResult)){std::printf("D3D9 device unavailable: 0x%08X\n",unsigned(deviceResult));return 2;}
 const auto scratch=output/L"ui-fixture";std::filesystem::create_directories(scratch/L"GT4Showroom");base=scratch.wstring()+L"\\";DeleteFileW((scratch/L"GT4Showroom/favorites.ini").c_str());
 const char*names[]={"sentinel","infernus","banshee","landstal","bravura","buffalo","cheetah","comet","elegant","sultan","elegy","camaro"};const char*display[]={"Sentinel","Infernus","Banshee","Landstalker","Bravura","Buffalo","Cheetah","Comet","Elegant","Sultan","Elegy","Camaro"};const int ids[]={405,411,429,400,401,402,415,480,507,560,562,15411};
 for(int n=0;n<12;++n){hostMeshes[ids[n]]=load(d,(n==11?camaro:shared)/(std::string(names[n])+".gmesh"));Entry e;e.id=ids[n];e.name=display[n];e.modelKey=modelNameKey(names[n]);e.country=n==11?Country::USA:defaultCountry(e);e.topSpeed=170+float(n*7);e.classId=n%4;e.category=classLabel(e.classId);e.information="Type:RWD    Mass:1500kg    Handling limit:"+std::to_string(int(e.topSpeed))+"km/h    Gears:5";for(uint32_t rgb:{0x35404B,0x9C2730,0xC6CED2,0x253A68}){Paint p;p.rgb[0]=rgb;p.rgb[1]=0x30363B;p.name=rgb==0x9C2730?"Cherry Red":rgb==0x253A68?"Police Car Blue":rgb==0xC6CED2?"Silver Stone Poly":"Midnight Blue";e.paints.push_back(p);}catalog.push_back(e);}
 entries=catalog;state.selected=11;state.begin(entries.size());assert(renderer.loadFont(font.wstring(),L"Michroma"));
 double worst=0,total=0;std::ofstream events(output/"preview-events.txt");
 for(int frame=0;frame<frames;++frame){hostKeys.fill(false);hostMouse.fill(0);PadActions pad;
  auto keyAt=[&](int at,int vk){if(frame==at){hostKeys[size_t(vk)]=true;events<<frame<<" key "<<vk<<"\n";}};
  if(frames==1){state.clock=5;state.changed=2;state.ui.pointerMoved=4;}
  else{
   keyAt(90,VK_UP);keyAt(110,VK_RIGHT);keyAt(120,VK_UP);keyAt(140,'F');keyAt(185,VK_RIGHT);
   keyAt(210,VK_RBUTTON);keyAt(220,VK_RIGHT);keyAt(230,VK_DOWN);keyAt(240,VK_DOWN);keyAt(250,VK_DOWN);keyAt(260,VK_RETURN);
   keyAt(290,VK_RBUTTON);keyAt(300,VK_DOWN);keyAt(310,VK_DOWN);keyAt(320,VK_RETURN);
   keyAt(350,VK_SPACE);
   if(frame==370){state.pointerX=(renderer.carHit.x+renderer.carHit.w*.5f)/1280;state.pointerY=(renderer.carHit.y+renderer.carHit.h*.55f)/720;hostKeys[VK_LBUTTON]=true;}
   if(frame>370&&frame<391){hostKeys[VK_LBUTTON]=true;game::at<float>(0xB73424)=7;game::at<float>(0xB73428)=-1;}
   if(frame==400||frame==403)game::at<uint8_t>(0xB7341B)=1;
   if(frame>=415&&frame<440){pad.orbitX=.65f;pad.orbitY=.2f;pad.zoom=-.2f;}
   keyAt(450,VK_BACK);keyAt(475,'F');
   if(frame==500)pad.search=true;
   const std::string query="CAMARO";for(int n=0;n<6;++n)if(frame==512+n*4){state.ui.keyboard.selected=int(query[size_t(n)]-'A');pad.accept=true;}
   if(frame==544)pad.favorites=true;
   if(frame==565)pad.accept=true;
   keyAt(580,VK_RIGHT);
   if(frame>=590){state.pointerMove(7,-5,1280,720);} // visibly undock, then a separate still checks idle return
  }
  hostFrame(pad,1./30);assert(state.open);assert(SUCCEEDED(d->BeginScene()));poison(d);assert(renderer.draw(d,1280,720,state,entries,[](int id){return lookup(id);}));state.presented();restored(d);assert(SUCCEEDED(d->EndScene()));
  worst=std::max(worst,renderer.cpuMs);total+=renderer.cpuMs;char file[40];std::snprintf(file,sizeof(file),"frame-%04d.png",frame);save(d,output/file);if(frame%60==0)std::printf("Rendered %d, mode favorite=%d cinematic=%d menu=%d search=%d\n",frame,state.ui.favoritesOnly,state.ui.cinematic.active,state.ui.menu.open,state.searching);
 }
 std::printf("PASS: shared production UI/renderer preview; %d frames, CPU submission %.3f mean / %.3f max ms. Native GTA/model plugins and physical controller hardware are not executed.\n",frames,total/std::max(1,frames),worst);
 if(frames<0){
  state.ui.resetTransient();state.begin(entries.size());state.clock=5;state.changed=0;
  auto draw=[&](UINT w,UINT h){assert(SUCCEEDED(d->BeginScene()));assert(renderer.draw(d,w,h,state,entries,[](int id){return lookup(id);}));assert(SUCCEEDED(d->EndScene()));};
  draw(1280,720);auto clean=pixels(d);poison(d);draw(1280,720);restored(d);assert(clean==pixels(d));
  std::puts("PASS: clean/hostile-state output pixel-identical; caller state restored.");
  double mean=0,maxMs=0;for(int i=0;i<150;++i){draw(1280,720);if(i>=30){mean+=renderer.cpuMs;maxMs=std::max(maxMs,renderer.cpuMs);}}
  std::printf("Warm renderer submission: mean %.4f max %.4f ms /120 samples, %llu triangles, %llu draw calls, %llu uploads. Not a game FPS benchmark.\n",mean/120,maxMs,(unsigned long long)renderer.triangles,(unsigned long long)renderer.drawCalls,(unsigned long long)renderer.uploads);
  for(auto size:std::array<std::pair<UINT,UINT>,4>{{{800,600},{1920,1080},{2560,1080},{5120,1440}}}){
   renderer.reset();pp.BackBufferWidth=size.first;pp.BackBufferHeight=size.second;assert(SUCCEEDED(d->Reset(&pp)));
   state.ui.menu.toggle(.8f,.8f,state.clock);state.ui.menu.row=0;state.ui.menu.sub=true;draw(size.first,size.second);char file[80];std::snprintf(file,sizeof(file),"menu-%ux%u.png",size.first,size.second);save(d,output/file);
   state.ui.menu.open=false;state.ui.cinematic.enter(state.clock,1);state.ui.cinematic.mix=1;draw(size.first,size.second);state.ui.cinematic={};
   state.searching=true;state.ui.keyboard.open=true;draw(size.first,size.second);state.searching=false;state.ui.keyboard.open=false;
   std::printf("PASS: menu, cinematic, controller keyboard and reset %ux%u.\n",size.first,size.second);
  }
  renderer.reset();pp.BackBufferWidth=800;pp.BackBufferHeight=600;
  if(SUCCEEDED(api->CheckDeviceMultiSampleType(0,D3DDEVTYPE_HAL,D3DFMT_X8R8G8B8,TRUE,D3DMULTISAMPLE_2_SAMPLES,nullptr))){pp.MultiSampleType=D3DMULTISAMPLE_2_SAMPLES;assert(SUCCEEDED(d->Reset(&pp)));draw(800,600);std::puts("PASS: 2x MSAA host target.");}
 }
 renderer.reset();hostMeshes.clear();d->Release();api->Release();DestroyWindow(window);Gdiplus::GdiplusShutdown(gdiplus);return 0;
}

