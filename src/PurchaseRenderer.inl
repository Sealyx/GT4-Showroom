void purchaseKeyArt(IDirect3DDevice9* d,float x,float y,float unit,float keyAlpha)
{
    d3dShopOverlay(d);
#ifndef GT4_ICON_EXPORT
    if(pngIcon(d,L"purchase-key.png",{x-102.4f*unit,y-76.8f*unit,204.8f*unit,153.6f*unit},keyAlpha)) return;
#endif
        struct Point {float x,y;};
        auto face=[&](std::initializer_list<Point> outline,DWORD shade,float dy=0.f) {
            std::array<ScreenVertex,24> vertices{};
            size_t n=0;
            for (auto p:outline) vertices[n++]={x+(p.x-150)*unit,y+(p.y-98+dy)*unit,0,1,color(shade & 0xffffff,keyAlpha),0,0};
            d->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,UINT(n-2),vertices.data(),sizeof(ScreenVertex));
        };
        const std::initializer_list<Point> blade{{149,86},{158,96},{81,148},{69,149},{64,143},{67,136}};
        const std::initializer_list<Point> grip{{135,92},{140,78},{146,65},{153,56},{164,49},{177,46},{194,46},{206,48},{216,53},{221,59},{222,68},{219,78},{214,86},{205,92},{193,96},{159,104},{146,103},{138,99}};
        // shade the raised face and bevel separately to keep the small key legible at native resolution
        auto silverFace=[&](std::initializer_list<Point> rim,Point center,DWORD light) {
            std::array<ScreenVertex,26> v{};
            const auto vertex=[&](Point p,DWORD tint) {
                return ScreenVertex{x+(p.x-150)*unit,y+(p.y-98)*unit,0,1,color(tint & 0xffffff,keyAlpha),0,0};
            };
            v[0]=vertex(center,light);
            size_t n=1;
            for(auto p:rim) {
                float lum=std::clamp(205.f-(p.y-center.y)*.96f-(p.x-center.x)*.29f,145.f,238.f);
                v[n++]=vertex(p,D3DCOLOR_XRGB(int(lum),int(lum-4),int(lum-8)));
            }
            v[n]=v[1];
            d->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,UINT(n-1),v.data(),sizeof(ScreenVertex));
        };
        face(blade,0xff514943,4);face(grip,0xff423b39,4);
        face(blade,0xffb4a9a0);face({{66,139},{145,87},{149,89},{70,142}},0xffe9e0d8);
        face({{71,143},{147,92},{153,92},{77,144}},0xff685e5a);
        face({{77,144},{152,94},{154,95},{82,146}},0xffe1d7cf);
        silverFace(grip,{177,65},0xfff3ece6);
        face({{139,94},{144,76},{153,60},{163,53},{177,49},{191,48},{168,54},{154,64},{145,81}},0xffeee6df);
        face({{149,98},{164,99},{196,91},{209,85},{216,77},{212,86},{203,92},{159,103}},0xff9b918b);
        // recessed keyring slot, with a polished lip catching light along its lower edge
        face({{196,51},{202,50},{211,53},{217,59},{217,63},{213,65},{206,62},{199,57}},0xffeee4dc);
        face({{198,52},{203,51},{211,54},{216,59},{215,62},{211,62},{203,58}},0xff645c59);
        face({{202,52},{208,54},{214,58},{211,59},{205,56}},0xff343233);
 }

void purchaseButtonBackground(IDirect3DDevice9* d,UIRect b,float scale,bool chosen,float alpha)
{
#ifndef GT4_ICON_EXPORT
    if(buttonBackdrop(d,chosen?"purchase-highlight":"purchase-normal",b,alpha))return;
#endif
            roundedRectangle(d,b.x,b.y+2*scale,b.w,b.h,b.h*.5f,color(0x514e50,alpha));
            roundedRectangle(d,b.x,b.y,b.w,b.h,b.h*.5f,chosen?color(0xe1bf81,alpha):color(0xd8d7de,alpha));
            roundedRectangle(d,b.x+scale,b.y+scale,b.w-2*scale,b.h-2*scale,b.h*.5f,chosen?color(0xc99e55,alpha):color(0xbab9c2,alpha));
            roundedRectangle(d,b.x+b.h*.23f,b.y+2*scale,b.w-b.h*.46f,b.h*.39f,b.h*.2f,chosen?color(0xe1bc7c,alpha):color(0xd1d0d8,alpha));
}

void drawPurchase(IDirect3DDevice9* d, float w, float h, const Model& s, const Entry& e, const Mesh* car)
{
    const auto& sequence = s.purchasePlate && s.plateOpen ? s.purchasePlateScene : s.purchase;
    const auto step = sequence.step;
    const bool studioView = step == PurchaseStep::Inspect || step == PurchaseStep::Invoice || step == PurchaseStep::Handoff;
    const float age = float(s.clock - sequence.began), scale = h / 720.f;
    double subjectAge=s.clock-sequence.blackBegan;
    bool aoReady=occlusionQuality==0;
    if(car && !studioView) {
        if(purchaseSubjectMesh!=car->serial || purchaseSubjectStage!=sequence.blackBegan) {
            purchaseSubjectMesh=car->serial;purchaseSubjectStage=sequence.blackBegan;purchaseSubjectBegan=-1;
        }
        if(auto* gpu=upload(d,*car)) {
            if(occlusionQuality>0) {
                const bool baked=prepareCavity(d,*car,*gpu,nullptr,0,8.);
                aoReady=gpu->cavityUnavailable || (baked && studioClock-gpu->cavityBegan>=.18);
#ifdef BODY_REVIEW
                if(baked && lastPurchaseCavityReady<0) lastPurchaseCavityReady=s.clock-sequence.blackBegan;
#endif
            }
        }
        if(aoReady && purchaseSubjectBegan<0) purchaseSubjectBegan=s.clock;
        subjectAge=purchaseSubjectBegan<0 ? 0 : s.clock-purchaseSubjectBegan;
    }
    Camera cam(w,h,5.f,studioView ? .48f : .49f);
    cam.offsetX = studioView ? 0 : -w * .19f;
    const float pitch = studioView ? .15f : .105f;
    const float vehicleReveal = ReviewMotion::smooth((subjectAge-.12)/.55) *
        (sequence.leaving>=0 ? 1-ReviewMotion::smooth((s.clock-sequence.leaving)/.55) : 1.f);
#ifdef BODY_REVIEW
    lastPurchaseOpacity=vehicleReveal;
    lastPurchaseAoReady=aoReady;
#endif
    const float distance = (studioView ? 7.25f : 8.0f) * (1.f+.085f*(1.f-vehicleReveal));
    const V3 focus{0,0,.72f};
    cam.eye = focus + V3{0,-std::cos(pitch)*distance,std::sin(pitch)*distance};
    cam.forward = unit(focus-cam.eye);
    cam.right = unit(cross(cam.forward,{0,0,1}));
    cam.up = cross(cam.right,cam.forward);
    if (!studioView) d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0);
    if (car && !studioView) {
        lightPhase=lastShadowPhase=float(s.clock-s.backdropPause)*.42f;
        const auto paint = appearancePaint(e,s,car);
        // Fit the body in the left-hand presentation area, then retain clearance for every accessory
        const bool twoWheels=e.kind==9 || e.kind==10;
        const V3 bodyHigh=car->raisedAccessory ? car->framingBodyHigh : previewFramingHigh(e,*car);
        const float bodyHeight=bodyHigh.z-car->low.z,bodyWidth=std::max(.1f,bodyHigh.x-car->low.x);
        const bool tallVehicle=bodyHeight>1.5f && bodyHeight/bodyWidth>.75f;
        const bool largeTruck=bodyHeight>2.3f && bodyHigh.y-car->low.y>5.8f;
        const float lowerShift=twoWheels ? .105f : largeTruck ? .055f : tallVehicle ? .020f : 0.f;
        if(twoWheels || tallVehicle) cam.cy+=h*(.025f+lowerShift);
        Camera fitCamera=cam;
        fitCamera.eye=focus+(cam.eye-focus)*(1.f/(1.f+.085f*(1.f-vehicleReveal)));
        const V3 center{(car->low.x+car->high.x)*.5f,(car->low.y+car->high.y)*.5f,car->low.z};
        const auto fits=[&](float size,V3 low,V3 high,float left,float top,float right,float bottom) {
            for(int corner=0;corner<8;++corner) {
                const V3 v{corner&1?high.x:low.x,corner&2?high.y:low.y,corner&4?high.z:low.z};
                const V3 point{-(v.x-center.x)*size,-(v.y-center.y)*size,(v.z-center.z)*size};
                float x=0,y=0,z=0,rhw=0;
                if(!fitCamera.project(point,x,y,z,rhw) || x<w*left || x>w*right || y<h*top || y>h*bottom) return false;
            }
            return true;
        };
        const auto fit=[&](V3 low,V3 high,float left,float top,float right,float bottom) {
            float lowSize=0,highSize=16.f/std::max(.1f,car->span());
            for(int iteration=0;iteration<22;++iteration) {
                const float mid=(lowSize+highSize)*.5f;
                if(fits(mid,low,high,left,top,right,bottom)) lowSize=mid;
                else highSize=mid;
            }
            return lowSize;
        };

        float bodyTop=.30f,bodyBottom=.80f,accessoryTop=.255f,accessoryBottom=.84f;
        if(twoWheels) {
            bodyTop=.12f;bodyBottom=.90f;accessoryTop=.10f;accessoryBottom=.94f;
        } else if(largeTruck) {
            bodyTop=.14f;bodyBottom=.88f;accessoryTop=.11f;accessoryBottom=.92f;
        } else if(tallVehicle) {
            bodyTop=.26f;bodyBottom=.84f;accessoryTop=.23f;accessoryBottom=.88f;
        }
        const float bodySize=fit(car->low,bodyHigh,.12f,bodyTop+lowerShift,.50f,bodyBottom+lowerShift);
        const float fullSize=fit(car->low,car->high,.04f,accessoryTop+lowerShift,.55f,std::min(.99f,accessoryBottom+lowerShift));
        const float size=std::min(bodySize,fullSize);
        const float angle = studioView ? pi + float(s.clock-sequence.opened)*.245f : pi;
        if (s.ui.floorReflections()>0) shopMirror(d,*car,cam,angle,paint,1,size,0,s.shop.washed,s.ui.floorReflections(),nullptr,studioView ? .42f : .18f);
        if (s.ui.shadows()) mesh(d,*car,cam,angle,paint,1,size,true);
        mesh(d,*car,cam,angle,paint,1,size);
        if(vehicleReveal<1.f) rect(d,0,0,w,h,color(0,1.f-vehicleReveal));
    }
    d3dShopOverlay(d);
    const auto write = [&](const std::string& msg,float x,float y,float pt,DWORD ink,bool centered=false) {
        text(d,msg,x*w,y*h,pt*scale,ink,centered,0,true);
    };
    if (!studioView) rect(d,0,0,w,h,color(0,.50f));
    const auto controls=[&](const PurchasePresentation& sequence,float dx,float alpha,float age,bool leaving) {
        const auto step=sequence.step;
        const auto write=[&](const char* msg,float x,float y,float pt,DWORD ink,bool center) {
            text(d,msg,x*w+dx,y*h,pt*scale,ink,center,0,true);
        };
    if(step==PurchaseStep::Offer) write("Use this vehicle now?",.767f,.415f,26,color(0xe2e2e2,alpha),true);
    else if(step==PurchaseStep::Processing) {
        write("Processing...",.757f,.427f,18,color(0xeeeeee,alpha),true);
        const float x=dx+w*.586f, y=h*.502f, width=w*.355f, height=h*.028f;
        const float inset=3*scale, fill=(width-2*inset)*ease(age/2.45f);
        roundedRectangle(d,x,y+2*scale,width,height,height*.5f,color(0x29272a,alpha));
        roundedRectangle(d,x,y,width,height,height*.5f,color(0xaaa9af,alpha));
        roundedRectangle(d,x+scale,y+scale,width-2*scale,height-2*scale,height*.5f,color(0x44434a,alpha));
        roundedRectangle(d,x+inset,y+inset,width-2*inset,height-2*inset,height*.5f,color(0x29282d,alpha));
        if(fill>0) {
            roundedRectangle(d,x+inset,y+inset,fill,height-2*inset,height*.5f,color(0xb27b2b,alpha));
            roundedRectangle(d,x+inset,y+inset,fill,(height-2*inset)*.64f,height*.4f,color(0xe3bc70,alpha));
            if(fill>height) rect(d,x+height*.5f,y+inset+scale,fill-height*.6f,scale,color(0xf3d99b,alpha));
        }
    } else if(step==PurchaseStep::Complete) write("Process complete",.757f,.427f,18,color(0xeeeeee,alpha),true);
    for(int i=0;i<sequence.buttons();++i) {
        auto b=sequence.button(i,w,h); b.x+=dx;
        if(step!=PurchaseStep::Inspect) {
            const char* caption=step==PurchaseStep::Invoice?(i?"Purchase":"Cancel"):
                                step==PurchaseStep::Offer?(i?"No":"Yes"):"OK";
            const bool chosen=i==sequence.choice;
            purchaseButtonBackground(d,b,scale,chosen,alpha);
            text(d,caption,b.x+b.w*.5f,b.y+b.h*.13f,22*scale,color(0x101013,alpha),true,0,true);
        }
        if(i==sequence.choice && s.ui.dock && !leaving && !s.plateOpen) cursor(d,b.x+b.w*.52f,b.y+b.h*.55f,scale*.85f,alpha);
    }
    };
    const auto leaveControls=[&](PurchaseStep prior,float progress) {
        if(progress>=1) return;
        PurchasePresentation previous;
        previous.step=prior;
        controls(previous,w*.16f*progress,1-progress,2.45f,true);
    };
    const float purchaseUiAlpha=s.purchasePlate && s.plateOpen ? 1.f-.5f*s.plateMotion.reveal(s.clock) : 1.f;
    if(!studioView && !(s.purchasePlate && s.plateOpen)) drawVehicleAccount(d,w,scale,e,s.previewPaint(e),s.dealerBalance);
    if (!studioView) {
        text(d,e.name,w*.50f,h*.066f,33*scale,color(0xeeeeee,purchaseUiAlpha),true);
        write("Vehicle selected for purchase.",.50f,.1575f,20,color(0xcccccc,purchaseUiAlpha),true);
    }
    const auto drawKey=[&] {
        if(step!=PurchaseStep::Inspect && step!=PurchaseStep::Invoice) return;
        if(step==PurchaseStep::Invoice && age>=.22f) return;
        const float keyAge=age;
        const float keyAlpha=step==PurchaseStep::Invoice ? 1-ease(keyAge/.22f) : ease(keyAge/.28f);
        const float keyOffset=28*scale*(1-keyAlpha) *
            (step==PurchaseStep::Invoice || sequence.previous==PurchaseStep::Invoice ? -1.f : 1.f);
        purchaseKeyArt(d,w*.5f,h*.782f+keyOffset,h/1080.f,keyAlpha);
        write("Purchase",.5f,.85f+keyOffset/h,16,color(0xeeeeee,keyAlpha),true);
    };
    const bool returningInvoice=step==PurchaseStep::Inspect && sequence.previous==PurchaseStep::Invoice && age<.22f;
    if (step==PurchaseStep::Invoice || step==PurchaseStep::Handoff || returningInvoice) {
        const float a=step==PurchaseStep::Invoice ? ease(age/.22f) : 1-ease(age/.22f);
        rect(d,w*.164f,h*.077f,w*.672f,h*.824f,color(0xbdbdc1,a*.79f));
        const DWORD rim=color(0x545459,a*.9f), highlight=color(0xf0f0f1,a*.62f);
        const float edge=std::max(1.f,scale);
        rect(d,w*.164f,h*.077f,w*.672f,edge,highlight);
        rect(d,w*.164f,h*.077f,edge,h*.824f,highlight);
        rect(d,w*.836f-edge,h*.077f,edge,h*.824f,rim);
        rect(d,w*.164f,h*.901f-edge,w*.672f,edge,rim);
        for(float line : {.170f,.420f,.569f,.675f,.805f}) {
            rect(d,w*.196f,h*line,w*.61f,edge,rim);
            rect(d,w*.196f,h*line+edge,w*.61f,edge,highlight);
        }
        for (float line : {.222f,.272f,.321f,.370f,.473f,.523f,.623f,.732f}) rect(d,w*.198f,h*line,w*.61f,edge,color(0x626266,a*.36f));
        for (auto block : {std::pair<float,float>{.222f,.420f},{.473f,.569f},{.623f,.732f}})
            rect(d,w*.5f,h*block.first,edge,h*(block.second-block.first),color(0x646469,a*.48f));
        const DWORD ink=color(0x101013,a);
        text(d,e.name,w*.50f,h*.083f,26*scale,ink,true);
        const auto row=[&](float y,const char* name,const std::string& val){
            write(name,.20f,y,20,ink);write(val,.515f,y,21,ink);
        };
        write("• Engine",.198f,.176f,22,ink);
        row(.231f,"Gears",e.handlingGears ? std::to_string(e.handlingGears) : "--");
        row(.280f,"Top speed",e.topSpeed>0 ? std::to_string(int(e.topSpeed))+" km/h" : "--");
        row(.329f,"Power",e.power.empty()?"----":e.power);
        row(.378f,"Torque",e.torque.empty()?"----":e.torque);
        write("• Dimensions and weight",.198f,.425f,22,ink);
        std::string dims="--";
        if(car){ const auto v=car->high-car->low;
            dims=std::to_string(int(v.y*1000))+" x "+std::to_string(int(v.x*1000))+" x "+std::to_string(int(v.z*1000)); }
        row(.482f,"Dimensions",dims);
        row(.530f,"Weight",e.handlingMass>0?std::to_string(int(e.handlingMass))+" kg":"--");
        write("• Body",.198f,.579f,22,ink);
        row(.631f,"Drivetrain",e.handlingDrive=='4'?"AWD":e.handlingDrive=='F'?"FWD":e.handlingDrive=='R'?"RWD":"--");
        write("• Price",.20f,.682f,20,ink);
        if(a>0) shopPriceMark(d,w*.527f,h*.699f,scale*.57f*a,true);
        write(shopMoney(e.price),.545f,.682f,21,ink);
        rect(d,w*.198f,h*.744f,w*.61f,h*.054f,color(0x4a4a4a,a));
        write("Your balance",.207f,.753f,18,color(0xffffff,a));
        if(a>0) shopPriceMark(d,w*.527f,h*.77f,scale*.62f*a,true);
        write(shopMoney(s.dealerBalance),.545f,.749f,23,color(0xffffff,a));
    }
    drawKey();
    const float progress=step>=PurchaseStep::Offer ? PurchasePresentation::slideProgress(age) : 1.f;
    if(step==PurchaseStep::Processing) leaveControls(PurchaseStep::Offer,progress);
    if(step==PurchaseStep::Complete) leaveControls(PurchaseStep::Processing,progress);
    controls(sequence,sequence.entryOffset(s.clock,w),progress*purchaseUiAlpha,age,false);
    if(s.plateOpen && s.purchasePlate) {
        drawPlate(d,w,h,s);keyboardPanel(d,w,h,s);
        if(s.pointerVisible && !s.ui.controllerUsed) cursor(d,s.pointerX*w,s.pointerY*h,scale*.85f,1);
    }
    if(!s.plateOpen && !s.ui.dock && s.pointerVisible) cursor(d,s.pointerX*w,s.pointerY*h,scale*.85f,1);
    if(step==PurchaseStep::Handoff) rect(d,0,0,w,h,color(0,ease((age-.45f)/.80f)));
    if(step==PurchaseStep::Offer && age<.65f) rect(d,0,0,w,h,color(0,1-ease(age/.65f)));
}
