void drawCountryPicker(IDirect3DDevice9* d,float W,float H,const Model& s) {
    const auto& p=s.countryPicker;if(!p.open)return;
    const float fade=std::clamp(float((s.clock-p.openedAt)/.08),0.f,1.f);
    uiFade=fade;
    d->SetPixelShader(nullptr);d->SetVertexShader(nullptr);d3dShopOverlay(d);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    const float scale=H/720.f;auto box=p.panel(W,H);const auto items=p.matches();
    messageWindow(d,box,"Countries",scale,1);
    auto field=messageRow(d,box,0,"Search","",scale,1);const auto icon=monitorArrowPanel(field,scale);monitorBevel(d,icon,1,scale,false,false,false);searchIcon(d,icon,scale,1);
    const float size=fitted(p.query,14.5f*scale,icon.x-field.x-18*scale,true);text(d,p.query,field.x+9*scale,field.y+4*scale,size,color(0x08080C),false,0,true);
    if(p.editing&&int(s.clock*2)%2==0)rect(d,field.x+9*scale+textWidth(p.query,size,true),field.y+4*scale,scale,15*scale,color(0x08080C));
    const auto g=p.grid(W,H);rect(d,g.x,g.y,g.w,g.h,color(0xA5A5AB));
    for(int i=p.firstRow*CountryPicker::columns;i<int(items.size())&&i<(p.firstRow+CountryPicker::visibleRows)*CountryPicker::columns;++i){
        const auto c=p.cell(W,H,i);const int id=items[size_t(i)];
        const float pop=p.editing?0.f:p.pop(s.clock,i);
        const float k=1.f+.12f*pop;
        const float cx=c.x+c.w*.5f;
        const float ballY=c.y+11*scale+22*scale,labelY=c.y+61*scale;
        const float anchor=(ballY+labelY)*.5f;
        const float cy=anchor+(ballY-anchor)*k,ly=anchor+(labelY-anchor)*k;
        const float diameter=44*scale*k;
        if(flags)rect(d,cx-diameter*.5f,cy-diameter*.5f,diameter,diameter,color(0xFFFFFF),flags,float(id%16)/16,float(id/16)/float((countryCount+15)/16),float(id%16+1)/16,float(id/16+1)/float((countryCount+15)/16));
        const std::string name=countryLabel(Country(id));
        text(d,name,cx,ly,fitted(name,12.5f*scale*k,c.w-10*scale,true),color(0x111114),true,0,true);
    }
    if(items.empty())text(d,"No countries found",g.x+g.w*.5f,g.y+g.h*.45f,15*scale,color(0x111114),true,0,true);
    if(p.maxRow(items.size())>0){const auto track=p.scrollTrack(W,H),thumb=p.scrollThumb(W,H,items.size());rect(d,track.x,track.y,track.w,track.h,color(0x8B8C92));monitorBevel(d,thumb,1,scale,p.scrolling,false,false);}
    monitorButton(d,{box.x+box.w*.5f-70*scale,box.y+box.h-34*scale,140*scale,22*scale},"Cancel",1,scale,p.cancelHot,1);
    if(s.ui.keyboard.open)keyboardPanel(d,W,H,s);
    uiFade=1.f;
}
