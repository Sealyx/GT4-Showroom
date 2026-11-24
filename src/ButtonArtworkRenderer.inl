bool buttonBackdrop(IDirect3DDevice9* device,const char* name,UIRect box,float alpha,bool sliced=true)
{
    const auto& image=buttonArtwork.get(device,name);if(!image.texture)return false;
    DWORD src=0,dst=0,enabled=0,minFilter=0,magFilter=0,mipFilter=0;
    device->GetRenderState(D3DRS_SRCBLEND,&src);device->GetRenderState(D3DRS_DESTBLEND,&dst);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE,&enabled);
    device->GetSamplerState(0,D3DSAMP_MINFILTER,&minFilter);device->GetSamplerState(0,D3DSAMP_MAGFILTER,&magFilter);
    device->GetSamplerState(0,D3DSAMP_MIPFILTER,&mipFilter);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    screen(device);device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
    const DWORD tint=color(0xffffff,alpha,alpha);
    if(sliced) {
        const float corner=std::string(name)=="prize-confirm"?.24f:.5f;
        const float cap=std::min(box.w*.5f,box.h*corner);
        const float u=std::min(.49f,float(image.height)*64.f/72.f*corner/image.width);
        const float height=box.h*72.f/64.f;
        rect(device,box.x,box.y,cap,height,tint,image.texture,0,0,u,1);
        rect(device,box.x+cap,box.y,std::max(0.f,box.w-2*cap),height,tint,image.texture,u,0,1-u,1);
        rect(device,box.x+box.w-cap,box.y,cap,height,tint,image.texture,1-u,0,1,1);
    } else rect(device,box.x,box.y,box.w,box.h,tint,image.texture);
    device->SetRenderState(D3DRS_SRCBLEND,src);device->SetRenderState(D3DRS_DESTBLEND,dst);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,enabled);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,minFilter);device->SetSamplerState(0,D3DSAMP_MAGFILTER,magFilter);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,mipFilter);return true;
}
