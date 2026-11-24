VehicleLogos::Image& iconImage(IDirect3DDevice9* device,const wchar_t* name)
{
    const std::wstring key(name);
    auto found=iconImages.find(key);
    if(found!=iconImages.end()) return found->second;
    const bool flag=key.compare(0,5,L"flag-")==0||key.compare(0,12,L"0flag-custom")==0;
    auto image=iconDictionary.load(device,assetPath(shopAssetRoot/L"icons.txd"),std::filesystem::path(key).stem().wstring());
    if(image.texture && !flag) {
        // Premultiplied GPU data keeps the transparent edges clean when icons are scaled or faded
        D3DLOCKED_RECT pixels{};
        if(SUCCEEDED(image.texture->LockRect(0,&pixels,nullptr,0))) {
            for(unsigned y=0;y<image.height;++y) {
                auto* row=static_cast<uint8_t*>(pixels.pBits)+y*pixels.Pitch;
                for(unsigned x=0;x<image.width;++x) for(unsigned channel=0;channel<3;++channel)
                    row[x*4+channel]=uint8_t((unsigned(row[x*4+channel])*row[x*4+3]+127)/255);
            }
            image.texture->UnlockRect(0);
        } else {
            image.texture->Release();image={};
        }
        if(image.texture) {
            IDirect3DTexture9* mip=nullptr;
            if(SUCCEEDED(device->CreateTexture(image.width,image.height,0,D3DUSAGE_AUTOGENMIPMAP,
                                               D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&mip,nullptr))) {
                D3DLOCKED_RECT source{},destination{};
                const bool sourceLocked=SUCCEEDED(image.texture->LockRect(0,&source,nullptr,D3DLOCK_READONLY));
                const bool destinationLocked=sourceLocked && SUCCEEDED(mip->LockRect(0,&destination,nullptr,0));
                if(destinationLocked) {
                    for(unsigned y=0;y<image.height;++y)
                        std::memcpy(static_cast<uint8_t*>(destination.pBits)+y*destination.Pitch,
                                    static_cast<const uint8_t*>(source.pBits)+y*source.Pitch,image.width*4);
                    mip->UnlockRect(0);
                }
                if(sourceLocked) image.texture->UnlockRect(0);
                if(destinationLocked) {
                    mip->GenerateMipSubLevels();
                    image.texture->Release();image.texture=mip;mip=nullptr;
                }
                if(mip) mip->Release();
            }
        }
    }
    return iconImages.emplace(key,std::move(image)).first->second;
}

bool pngIcon(IDirect3DDevice9* device,const wchar_t* name,UIRect box,float alpha,uint32_t tint=0xffffff,
             float angle=0,float pivotX=0,float pivotY=0)
{
    if(alpha<=0) return true;
    auto& image=iconImage(device,name);
    if(!image.texture) return false;
    DWORD source=0,destination=0,enabled=0,minFilter=0,magFilter=0,mipFilter=0;
    device->GetRenderState(D3DRS_SRCBLEND,&source);
    device->GetRenderState(D3DRS_DESTBLEND,&destination);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE,&enabled);
    device->GetSamplerState(0,D3DSAMP_MINFILTER,&minFilter);
    device->GetSamplerState(0,D3DSAMP_MAGFILTER,&magFilter);
    device->GetSamplerState(0,D3DSAMP_MIPFILTER,&mipFilter);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);
    bool drawn=false;
    if(!angle) drawn=rect(device,box.x,box.y,box.w,box.h,color(tint,alpha,alpha),image.texture);
    else {
        screen(device);
        ScreenVertex v[]={{box.x-.5f,box.y-.5f,0,1,color(tint,alpha,alpha),0,0},
                          {box.x+box.w-.5f,box.y-.5f,0,1,color(tint,alpha,alpha),1,0},
                          {box.x+box.w-.5f,box.y+box.h-.5f,0,1,color(tint,alpha,alpha),1,1},
                          {box.x-.5f,box.y+box.h-.5f,0,1,color(tint,alpha,alpha),0,1}};
        const float sine=std::sin(angle),cosine=std::cos(angle);
        for(auto& point:v) {const float x=point.x-pivotX,y=point.y-pivotY;
            point.x=pivotX+x*cosine-y*sine;point.y=pivotY+x*sine+y*cosine;}
        device->SetTexture(0,image.texture);
        device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
        device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
        drawn=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,v,sizeof(ScreenVertex)));
    }
    device->SetRenderState(D3DRS_SRCBLEND,source);
    device->SetRenderState(D3DRS_DESTBLEND,destination);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,enabled);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,minFilter);
    device->SetSamplerState(0,D3DSAMP_MAGFILTER,magFilter);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,mipFilter);
    return drawn;
}

bool pngCurrency(IDirect3DDevice9* device,float x,float y,float height,uint32_t tint,float tilt=0)
{
    auto& glyph=iconImage(device,L"currency-dollar.png");
    if(!glyph.texture || glyph.width<2 || !glyph.height) return false;
    const float sy=height/(glyph.height*.75f),sx=sy*1.08f;
    return pngIcon(device,L"currency-dollar.png",
                   {x-(glyph.width-1)*sx*.5f,y,glyph.width*sx,glyph.height*sy},1,tint,tilt,x,y+height*.5f);
}

bool pngCursor(IDirect3DDevice9* device,float x,float y,float scale,float alpha)
{
    auto& gold=iconImage(device,L"cursor-gold.png");
    auto& silver=iconImage(device,L"cursor-silver.png");
    auto& shadow=iconImage(device,L"cursor-shadow.png");
    if(!gold.texture || !silver.texture || !shadow.texture) return false;
    D3DCAPS9 caps{};
    if(FAILED(device->GetDeviceCaps(&caps)) || caps.MaxTextureBlendStages<3) return false;
    const float feedback=std::clamp(cursorFeedback,0.f,1.f),angle=feedback*pi/6;
    const float co=std::cos(angle),si=std::sin(angle);
    auto vertices=[&](float offset) {
        const float left=-12.8f*scale,top=-6.4f*scale,side=51.2f*scale;
        std::array<ScreenVertex,4> v{};
        constexpr std::array<std::array<float,2>,4> uv{{{0,0},{1,0},{1,1},{0,1}}};
        for(size_t i=0;i<v.size();++i) {
            const float px=left+uv[i][0]*side,py=top+uv[i][1]*side;
            v[i]={x+px*co-py*si+offset-.5f,y+px*si+py*co+offset-.5f,0,1,
                  color(0xffffff,alpha,alpha),uv[i][0],uv[i][1]};
        }
        return v;
    };
    screen(device);
    DWORD source=0,destination=0,enabled=0;
    device->GetRenderState(D3DRS_SRCBLEND,&source);
    device->GetRenderState(D3DRS_DESTBLEND,&destination);
    device->GetRenderState(D3DRS_ALPHABLENDENABLE,&enabled);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    std::array<std::array<DWORD,3>,2> filters{};
    constexpr D3DSAMPLERSTATETYPE filterTypes[]{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER};
    for(unsigned sampler=0;sampler<2;++sampler) for(unsigned i=0;i<3;++i) {
        device->GetSamplerState(sampler,filterTypes[i],&filters[sampler][i]);
        device->SetSamplerState(sampler,filterTypes[i],D3DTEXF_LINEAR);
    }
    device->SetTexture(0,shadow.texture);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    auto quad=vertices(1.4f*scale);
    bool okay=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,quad.data(),sizeof(ScreenVertex)));
    device->SetTexture(0,gold.texture);device->SetTexture(1,silver.texture);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,0);
    device->SetTextureStageState(1,D3DTSS_CONSTANT,color(0xffffff,feedback,feedback));
    for(auto operation:{D3DTSS_COLOROP,D3DTSS_ALPHAOP}) device->SetTextureStageState(1,operation,D3DTOP_LERP);
    for(auto argument:{D3DTSS_COLORARG0,D3DTSS_ALPHAARG0}) device->SetTextureStageState(1,argument,D3DTA_CONSTANT);
    for(auto argument:{D3DTSS_COLORARG1,D3DTSS_ALPHAARG1}) device->SetTextureStageState(1,argument,D3DTA_TEXTURE);
    for(auto argument:{D3DTSS_COLORARG2,D3DTSS_ALPHAARG2}) device->SetTextureStageState(1,argument,D3DTA_CURRENT);
    for(auto operation:{D3DTSS_COLOROP,D3DTSS_ALPHAOP}) device->SetTextureStageState(2,operation,D3DTOP_MODULATE);
    for(auto argument:{D3DTSS_COLORARG1,D3DTSS_ALPHAARG1}) device->SetTextureStageState(2,argument,D3DTA_CURRENT);
    for(auto argument:{D3DTSS_COLORARG2,D3DTSS_ALPHAARG2}) device->SetTextureStageState(2,argument,D3DTA_DIFFUSE);
    quad=vertices(0);
    okay=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,2,quad.data(),sizeof(ScreenVertex))) && okay;
    device->SetTexture(1,nullptr);
    for(unsigned stage=1;stage<=2;++stage) for(auto operation:{D3DTSS_COLOROP,D3DTSS_ALPHAOP})
        device->SetTextureStageState(stage,operation,D3DTOP_DISABLE);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
    device->SetRenderState(D3DRS_SRCBLEND,source);
    device->SetRenderState(D3DRS_DESTBLEND,destination);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,enabled);
    for(unsigned sampler=0;sampler<2;++sampler) for(unsigned i=0;i<3;++i)
        device->SetSamplerState(sampler,filterTypes[i],filters[sampler][i]);
    drawCalls+=2;
    return okay;
}
