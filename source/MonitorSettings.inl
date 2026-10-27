inline void ContextMenu::monitorToggle(float px,float py,double now){
 if(open){finish(-1);return;}open=true;closeResult=0;sliderDrag=-1;sliderChanged=false;sub=countries=countryFocus=false;row=sortRow=countryRow=focusLevel=0;
 monitorPicker=monitorHover=monitorHeaderHover=monitorKeyboard=false;monitorDragLevel=monitorHelpLevel=monitorHelpIndex=monitorHoverLevel=monitorHoverIndex=monitorFooter=-1;monitorPlaced={};
 x=px;y=py;began=monitorChanged=now;
}
inline int ContextMenu::monitorStart(int level,float h)const{
 const int rows=std::max(1,int(items(level).size()));
 const int visible=std::max(1,int((h*.76f)/(26*(h/720.f))));
 return std::clamp(current(level)-visible+1,0,std::max(0,rows-visible));
}
inline std::array<UIRect,3> ContextMenu::monitorPanels(float w,float h)const{
 const float s=h/720.f,margin=8*s;
 auto bound=[&](UIRect r){r.w=std::min(r.w,w-margin*2);r.h=std::min(r.h,h*.86f);r.x=std::clamp(r.x,margin,std::max(margin,w-r.w-margin));r.y=std::clamp(r.y,margin,std::max(margin,h*.9f-r.h-margin));return r;};
 std::array<UIRect,3> out{};
 out[0]=bound({x*w,y*h,480*s,(22+28*rootItems().size()+35)*s});
 const auto parent=out[0];
 const float childX=parent.x-372*s>=margin?parent.x-372*s:parent.x+parent.w+12*s;
 if(monitorSubWindow())out[1]=bound({childX,parent.y+22*s,360*s,(22+28*std::max(size_t(1),items(1).size())+35)*s});
 else out[1]=bound({parent.x+parent.w*.49f,parent.y+(22+28*row)*s, parent.w*.49f,std::min(h*.76f,(2+26*std::max(size_t(1),items(1).size()))*s)});
 out[2]=bound({childX,parent.y+(22+28*row)*s,370*s,88*s});
 for(int i=1;i<3;++i)if(monitorPlaced[size_t(i)]){out[size_t(i)].x=monitorX[size_t(i)]*w;out[size_t(i)].y=monitorY[size_t(i)]*h;out[size_t(i)]=bound(out[size_t(i)]);}
 return out;
}
inline UIRect ContextMenu::monitorField(int level,int index,float w,float h)const{
 const auto r=monitorPanels(w,h)[size_t(level)];const float s=h/720.f;
 const float ratio=level==0?.49f:.43f;return {r.x+r.w*ratio,r.y+(25+28*index)*s,r.w*(1-ratio)-12*s,21*s};
}
inline UIRect ContextMenu::monitorButton(int level,int which,float w,float h)const{
 const auto r=monitorPanels(w,h)[size_t(level)];const float s=h/720.f;
 const float bw=r.w*.34f;return {r.x+r.w*(which?.55f:.11f),r.y+r.h-26*s,bw,18*s};
}
inline bool ContextMenu::monitorFieldSelected(int level,int index,bool single)const{
 if(monitorDragLevel>=0)return false;
 return monitorKeyboard?(monitorFooter<0&&focus()==level&&(single||current(level)==index)):(monitorHoverLevel==level&&monitorHoverIndex==index);
}
inline bool ContextMenu::monitorButtonSelected(int level,int which)const{
 return monitorDragLevel<0&&monitorFooter==which&&monitorFooterLevel==level&&(monitorKeyboard||monitorHover);
}
inline UIRect ContextMenu::monitorDropdown(float w,float h)const{
 const auto r=monitorField(monitorSubWindow()?1:2,monitorSubWindow()?sortRow:0,w,h);const float s=h/720.f;
 const float hh=std::min(h*.76f,(2+26*std::max(size_t(1),items(2).size()))*s);
 const float ww=std::max(r.w,220*s),xx=std::clamp(r.x,8*s,std::max(8*s,w-ww-8*s));
 float yy=r.y+r.h;if(yy+hh>h*.9f)yy=std::max(8*s,r.y-hh);
 return {xx,yy,ww,hh};
}
inline UIRect ContextMenu::pointerTarget(float w,float h)const{
 const int level=focus();
 if(monitorFooter>=0)return monitorButton(monitorFooterLevel,monitorFooter,w,h);
 if(level==2){if(!monitorPicker)return monitorField(monitorSubWindow()?1:2,monitorSubWindow()?sortRow:0,w,h);const auto r=monitorDropdown(w,h);return monitorListRow(r,2,countryRow,h);}
 if(level==1&&!monitorSubWindow()){const auto r=monitorPanels(w,h)[1];return monitorListRow(r,1,sortRow,h);}
 return monitorField(level,current(level),w,h);
}
inline std::string ContextMenu::monitorTitle(int level)const{
 if(level==0)return "Showroom Settings";
 const auto branch=level==1?childBranch():thirdBranch();
 switch(branch){case MenuBranch::Volume:return "Volume";case MenuBranch::CinematicControls:return "Cinematic Controls";case MenuBranch::Countries:return "Countries";case MenuBranch::Classes:return "Vehicle Classes";case MenuBranch::Logos:return "Logo Settings";case MenuBranch::Extras:return "Extra Settings";case MenuBranch::Display:return "Display Settings";case MenuBranch::BrandLogo:return "Manufacturer Logo";case MenuBranch::CarLogo:return "Car Name Logo";case MenuBranch::Extra1:return "Vanilla Extras";case MenuBranch::Extra2:return "VehFuncs Extras";default:return "Settings";}
}
inline std::string ContextMenu::monitorValue(int level,int index)const{
 const auto rows=items(level);if(index<0||size_t(index)>=rows.size())return {};
 const auto&item=rows[size_t(index)];const auto branch=item.child;
 if(branch!=MenuBranch::None){
  if(branch==MenuBranch::Sort){static const char* labels[]={"Ascending A - Z","Descending Z - A","Class","Top Speed","Countries","Model ID"};return labels[std::clamp(order,0,5)];}
  if(branch==MenuBranch::Countries)return countryFilter<0?"All Countries":countryFilter==0?"Unassigned":countryLabel(Country(countryFilter));
  if(branch==MenuBranch::Classes)return classFilter<0?"All Classes":classNames[size_t(classFilter)];
  if(branch==MenuBranch::Cinematic)return cinematicView?"On":"Off";
  if(branch==MenuBranch::Favorites)return favoriteView?"Favorites":"All Cars";
  if(branch==MenuBranch::Hotspots)return hotspots?"On":"Off";
  if(branch==MenuBranch::Lights)return lights?"On":"Off";
  if(branch==MenuBranch::Indicators){static const char* modes[]={"Off","Left","Right","Hazard"};return modes[std::clamp(indicators,0,3)];}
  if(branch==MenuBranch::Shadows)return shadows?"On":"Off";
  if(branch==MenuBranch::BrandLogo)return brandLogo?"On":"Off";
  if(branch==MenuBranch::CarLogo)return carLogo?"On":"Off";
  if(branch==MenuBranch::DisplayModelId)return displayModelId?"On":"Off";
  if(branch==MenuBranch::DisplayModelName)return displayModelName?"On":"Off";
  if(branch==MenuBranch::DisplayClass)return displayClass?"On":"Off";
  if(branch==MenuBranch::Extra1||branch==MenuBranch::Extra2){const size_t group=branch==MenuBranch::Extra2?1:0;const auto&values=extras[group];return values.empty()?"Default":values[size_t(std::clamp(extraChoice[group],0,int(values.size())-1))];}
  return "Settings";
 }
 if(item.action==StudioAction::Favorites||item.action==StudioAction::AllCars)return favoriteView?"Favorites":"All Cars";
 if(item.action==StudioAction::Cinematic)return cinematicView?"On":"Off";
 return item.label;
}
inline std::string ContextMenu::help()const{
 if(!open||(!monitorHover&&monitorDragLevel<0))return {};
 if(monitorDragLevel>=0||monitorHeaderHover)return "Drag the title bar to move this settings window";
 if(monitorFooter>=0)return monitorFooter?"Close this settings window":monitorFooterLevel?"Return to the previous settings window":"Close showroom settings";
 const int level=monitorHelpLevel>=0?monitorHelpLevel:focus();const auto rows=items(level);const int index=monitorHelpLevel>=0?monitorHelpIndex:current(level);if(index<0||size_t(index)>=rows.size())return {};
 const auto item=rows[size_t(index)];const auto branch=item.child;
 if(branch==MenuBranch::Volume)return "Adjust master, music and sound effect volume";
 if(childBranch()==MenuBranch::Volume)return "Drag the slider or use Left / Right to adjust volume";
 if(branch==MenuBranch::CinematicControls)return "Adjust cinematic hotspots, lights and indicators";
 if(branch==MenuBranch::Hotspots)return "Show interactive vehicle hotspots in cinematic mode";
 if(branch==MenuBranch::Lights)return "Switch vehicle lights on or off without starting the engine";
 if(branch==MenuBranch::Indicators)return "Choose left, right or hazard indicators";
 if(branch==MenuBranch::Cinematic)return "Enable or disable cinematic mode; keep settings open until OK or Cancel";
 if(branch==MenuBranch::Favorites)return "Switch between favorites and the complete vehicle list";
 if(branch==MenuBranch::Sort)return "Choose the order of the vehicle list";
 if(branch==MenuBranch::Countries||item.action==StudioAction::FilterCountry)return "Show vehicles from the selected country";
 if(branch==MenuBranch::Classes||item.action==StudioAction::FilterClass)return "Show vehicles from the selected class";
 if(branch==MenuBranch::Shadows||item.action==StudioAction::Shadows)return "Switch the studio vehicle shadow on or off";
 if(branch==MenuBranch::Display)return "Choose the information shown above the preview vehicle";
 if(branch==MenuBranch::DisplayModelId||item.action==StudioAction::DisplayModelId)return "Show the selected vehicle model ID above its preview";
 if(branch==MenuBranch::DisplayModelName||item.action==StudioAction::DisplayModelName)return "Show the registered vehicle model name above its preview";
 if(branch==MenuBranch::DisplayClass||item.action==StudioAction::DisplayClass)return "Show the selected vehicle class above its preview";
 if(branch==MenuBranch::Logos)return "Choose the manufacturer and car name logo display";
 if(branch==MenuBranch::BrandLogo||item.action==StudioAction::BrandLogo)return "Use a supplied manufacturer logo when available";
 if(branch==MenuBranch::CarLogo||item.action==StudioAction::CarLogo)return "Use a supplied car name logo when available";
 if(branch==MenuBranch::Extras)return "Choose an available extra configuration";
 if(branch==MenuBranch::Extra1||item.action==StudioAction::Extra1)return "Preview and apply an available vanilla extra";
 if(branch==MenuBranch::Extra2||item.action==StudioAction::Extra2)return "Preview and apply one VehFuncs extra configuration";
 switch(item.action){case StudioAction::SortAscending:return "Order vehicles by name from A to Z";case StudioAction::SortDescending:return "Order vehicles by name from Z to A";case StudioAction::SortSpeed:return "Order vehicles by their top speed";case StudioAction::SortModelId:return "Order vehicles by model ID";case StudioAction::Favorites:case StudioAction::AllCars:return "Switch between favorites and the complete vehicle list";case StudioAction::Cinematic:return "Orbit and zoom around the selected vehicle";default:return {};}
}
inline UIRect ContextMenu::monitorListRow(UIRect list,int level,int index,float h)const {
 const float edge=std::max(.6f,h/720.f),slot=26*h/720.f;
 const float top=list.y+edge+(index-monitorStart(level,h))*slot;
 const float bottom=std::min(top+slot,list.y+list.h-edge);
 if(top<list.y+edge-.01f||bottom<=top)return {};
 return {list.x+edge,top,std::max(0.f,list.w-2*edge),bottom-top};
}
inline bool ContextMenu::fieldOpensWindow(int level,int index)const {
 const auto rows=items(level);if(index<0||size_t(index)>=rows.size())return false;
 const auto b=rows[size_t(index)].child;
 return b==MenuBranch::Display||b==MenuBranch::Logos||b==MenuBranch::Extras||b==MenuBranch::Volume||b==MenuBranch::CinematicControls;
}
inline int ContextMenu::audibleTarget()const {
 if(!open||monitorDragLevel>=0||monitorHeaderHover)return -1;
 if(monitorFooter>=0)return 10000+monitorFooterLevel*2+monitorFooter;
 if(monitorKeyboard)return focus()*1000+current(focus());
 if(monitorHoverLevel>=0&&monitorHoverIndex>=0)return monitorHoverLevel*1000+monitorHoverIndex;
 return -1;
}
inline UIRect ContextMenu::monitorSlider(int index,float w,float h)const {
 const auto field=monitorField(1,index,w,h);const float scale=h/720.f;
 return {field.x+9*scale,field.y+3*scale,std::max(1.f,field.w-60*scale),field.h-6*scale};
}
inline bool ContextMenu::monitorSlide(float px,float py,float w,float h,bool down,bool pressed) {
 if(!open||!sub||childBranch()!=MenuBranch::Volume){sliderDrag=-1;return false;}
 if(!down){sliderDrag=-1;return false;}
 if(pressed)for(int i=0;i<3;++i)if(monitorField(1,i,w,h).contains(px,py)){sliderDrag=i;break;}
 if(sliderDrag<0)return false;
 const auto track=monitorSlider(sliderDrag,w,h);
 const int value=int(std::lround(std::clamp((px-track.x)/track.w,0.f,1.f)*100));
 sliderChanged|=volumes.value[size_t(sliderDrag)]!=value;volumes.value[size_t(sliderDrag)]=value;
 sortRow=sliderDrag;focusLevel=1;monitorKeyboard=false;monitorHover=true;monitorHoverLevel=1;monitorHoverIndex=sliderDrag;
 monitorFooter=-1;monitorPicker=false;return true;
}
inline void ContextMenu::monitorPoint(float px,float py,float w,float h) {
 monitorHover=monitorHeaderHover=monitorKeyboard=false;
 monitorHelpLevel=monitorHelpIndex=monitorHoverLevel=monitorHoverIndex=monitorFooter=-1;
 if(monitorDragLevel>=0||sliderDrag>=0){monitorHover=true;return;}
 const float scale=h/720.f;const auto boxes=monitorPanels(w,h);
 auto footer=[&](int level){for(int button=0;button<2;++button)if(monitorButton(level,button,w,h).contains(px,py)){monitorFooterLevel=level;monitorFooter=button;monitorHover=true;return true;}return false;};
 auto field=[&](int level,int index){if(!monitorField(level,index,w,h).contains(px,py))return false;monitorHoverLevel=level;monitorHoverIndex=index;return true;};
 auto list=[&](UIRect box,int level){
  if(!box.contains(px,py))return false;
  const auto rows=items(level);
  for(int i=monitorStart(level,h);i<int(rows.size());++i)if(monitorListRow(box,level,i,h).contains(px,py)){
   if(level==2){countryRow=i;countryFocus=true;}else{sortRow=i;countryFocus=false;}
   focusLevel=level;monitorHover=true;monitorHoverLevel=level;monitorHoverIndex=i;return true;
  }
  return true;
 };
 if(monitorPicker&&list(monitorDropdown(w,h),2))return;
 if(monitorSelectorWindow()){
  const auto box=boxes[2];if(box.contains(px,py)){if(footer(2))return;monitorHeaderHover=py<box.y+22*scale;field(2,0);monitorHover=monitorHeaderHover||monitorHoverLevel==2;return;}
 }
 if(sub){
  if(monitorSubWindow()){
   const auto box=boxes[1];if(box.contains(px,py)){
    if(footer(1))return;monitorHeaderHover=py<box.y+22*scale;monitorHover=monitorHeaderHover;
    const int i=int((py-box.y-22*scale)/(28*scale));
    if(py>=box.y+22*scale&&i>=0&&size_t(i)<items(1).size()){
     monitorHelpLevel=1;monitorHelpIndex=i;monitorHover=true;
     if(field(1,i)){if(sortRow!=i){monitorPicker=false;countryFocus=false;}sortRow=i;focusLevel=1;countryFocus=false;}
    }
    return;
   }
  }else if(!monitorSelectorWindow()&&list(boxes[1],1))return;
 }
 const auto box=boxes[0];if(!box.contains(px,py))return;if(footer(0))return;
 monitorHeaderHover=py<box.y+22*scale;monitorHover=monitorHeaderHover;
 const int i=int((py-box.y-22*scale)/(28*scale));
 if(py>=box.y+22*scale&&i>=0&&size_t(i)<rootItems().size()){
  monitorHelpLevel=0;monitorHelpIndex=i;monitorHover=true;
  if(field(0,i)&&!sub){if(row!=i){row=i;sortRow=countryRow=0;countryFocus=monitorPicker=false;}focusLevel=0;countryFocus=false;}
 }
}
inline void ContextMenu::monitorBack(){
 monitorHover=true;monitorHeaderHover=false;monitorHelpLevel=monitorHelpIndex=monitorHoverLevel=monitorHoverIndex=monitorFooter=-1;
 if(monitorPicker){monitorPicker=false;if(monitorSubWindow()){countryFocus=false;focusLevel=1;}return;}
 if(focus()==2){countryFocus=false;focusLevel=1;return;}
 if(sub){sub=countryFocus=false;focusLevel=0;return;}
 finish(-1);
}
inline void ContextMenu::monitorMove(int dx,int dy){
 if(!open)return;monitorHover=monitorKeyboard=true;monitorHeaderHover=false;monitorHelpLevel=monitorHelpIndex=monitorHoverLevel=monitorHoverIndex=-1;
 const int level=monitorFooter>=0?monitorFooterLevel:focus();
 if(monitorFooter>=0){
  if(dx)monitorFooter=std::clamp(monitorFooter+dx,0,1);
  if(dy){monitorFooter=-1;focusLevel=level;countryFocus=level==2;if(level==0){row=dy<0?int(rootItems().size())-1:0;sub=false;}else if(level==1)sortRow=dy<0?int(items(1).size())-1:0;}
  return;
 }
 if(level==1&&childBranch()==MenuBranch::Volume&&dx){
  if(sortRow>=0&&sortRow<3){const int previous=volumes.value[size_t(sortRow)];volumes.value[size_t(sortRow)]=std::clamp(previous+dx*5,0,100);sliderChanged|=volumes.value[size_t(sortRow)]!=previous;}return;
 }
 if(dx<0){monitorBack();return;}
 if(level==2){if(!monitorPicker){if(dx>0)monitorPicker=true;else if(dy){monitorFooter=dy>0?0:1;monitorFooterLevel=level;}return;}const int n=int(items(2).size());if(n)countryRow=(countryRow+dy%n+n)%n;return;}
 if(dx>0){const auto rows=items(level);const int index=current(level);if(index>=0&&size_t(index)<rows.size()&&rows[size_t(index)].child!=MenuBranch::None)monitorActivate(favoriteView);return;}
 const int n=int(items(level).size());if(!n)return;
 if(level==0){if(row+dy>=n||row+dy<0){monitorFooter=dy>0?0:1;monitorFooterLevel=level;sub=countryFocus=monitorPicker=false;}else{row=(row+dy%n+n)%n;sortRow=countryRow=0;sub=countryFocus=monitorPicker=false;}}
 else {if(monitorSubWindow()&&(sortRow+dy>=n||sortRow+dy<0)){monitorFooter=dy>0?0:1;monitorFooterLevel=level;}else sortRow=(sortRow+dy%n+n)%n;countryRow=0;monitorPicker=countryFocus=false;}
}
inline StudioAction ContextMenu::monitorActivate(bool favorites){
 favoriteView=favorites;if(!open)return StudioAction::None;monitorHover=true;monitorHelpLevel=monitorHelpIndex=-1;
 const int level=monitorFooter>=0?monitorFooterLevel:focus();if(monitorFooter>=0){const int button=monitorFooter;monitorFooter=-1;monitorPicker=false;if(level==0){finish(button==0?-1:1);}else if(level==1){sub=countryFocus=false;focusLevel=0;}else monitorBack();return StudioAction::None;}const auto rows=items(level);const int index=current(level);if(index<0||size_t(index)>=rows.size())return StudioAction::None;
 if(level==2&&!monitorPicker){monitorPicker=true;return StudioAction::None;}
 const auto item=rows[size_t(index)];
 if(item.child!=MenuBranch::None){
  if(level==0){sub=true;focusLevel=1;countryFocus=false;sortRow=0;monitorPlaced[1]=false;const auto options=items(1);for(size_t i=0;i<options.size();++i)if(options[i].checked){sortRow=int(i);break;}}
  else {countryFocus=true;focusLevel=2;countryRow=0;monitorPicker=monitorSubWindow();monitorPlaced[2]=false;const auto options=items(2);for(size_t i=0;i<options.size();++i)if(options[i].checked){countryRow=int(i);break;}}
  countries=thirdBranch()==MenuBranch::Countries;return StudioAction::None;
 }
 if(level==1&&childBranch()==MenuBranch::Volume)return StudioAction::None;
 choice=item.value;
 if(item.action==StudioAction::Cinematic){sub=countryFocus=monitorPicker=false;focusLevel=0;return item.action;}
 if(level==2&&monitorSubWindow()){monitorPicker=false;countryFocus=false;focusLevel=1;}
 else if(item.action==StudioAction::Shadows){sub=false;focusLevel=0;countryFocus=false;}
 else {open=sub=countries=countryFocus=monitorPicker=false;}
 return item.action;
}
inline StudioAction ContextMenu::monitorClick(float px,float py,float w,float h,bool favorites){
 if(!open)return StudioAction::None;monitorPoint(px,py,w,h);const float s=h/720.f;const auto boxes=monitorPanels(w,h);
 auto pick=[&](UIRect box,int level){const auto rows=items(level);for(int i=monitorStart(level,h);i<int(rows.size());++i)if(monitorListRow(box,level,i,h).contains(px,py)){if(level==2){countryRow=i;countryFocus=true;}else{sortRow=i;countryFocus=false;}focusLevel=level;return true;}return false;};
 if(monitorPicker&&pick(monitorDropdown(w,h),2))return monitorActivate(favorites);
 if(monitorSelectorWindow()){
  if(monitorField(2,0,w,h).contains(px,py)){monitorPicker=!monitorPicker;return StudioAction::None;}
  for(int i=0;i<2;++i)if(monitorButton(2,i,w,h).contains(px,py))return monitorActivate(favorites);
  if(boxes[2].contains(px,py))return StudioAction::None;
 }
 if(sub){if(monitorSubWindow()){
   for(size_t i=0;i<items(1).size();++i)if(monitorField(1,int(i),w,h).contains(px,py)){sortRow=int(i);focusLevel=1;countryFocus=false;return monitorActivate(favorites);}
   for(int i=0;i<2;++i)if(monitorButton(1,i,w,h).contains(px,py))return monitorActivate(favorites);
   if(boxes[1].contains(px,py))return StudioAction::None;
  }else if(!monitorSelectorWindow()&&pick(boxes[1],1))return monitorActivate(favorites);
 }
 for(size_t i=0;i<rootItems().size();++i)if(monitorField(0,int(i),w,h).contains(px,py)){
  row=int(i);focusLevel=0;countryFocus=sub=monitorPicker=false;return monitorActivate(favorites);
 }
 for(int i=0;i<2;++i)if(monitorButton(0,i,w,h).contains(px,py))return monitorActivate(favorites);
 if(boxes[0].contains(px,py))return StudioAction::None;
 if(monitorPicker){monitorPicker=false;return StudioAction::None;}if(sub){sub=countryFocus=false;focusLevel=0;}else if(!boxes[0].contains(px,py))finish(-1);
 (void)s;return StudioAction::None;
}
inline bool ContextMenu::monitorDrag(float px,float py,float w,float h,bool down,bool pressed){
 if(!open)return false;if(!down){monitorDragLevel=-1;return false;}
 const float s=h/720.f;const auto boxes=monitorPanels(w,h);
 if(pressed){
  // Visible dropdowns own their pixels, including an underlying title bar.
  if((monitorPicker&&monitorDropdown(w,h).contains(px,py))||(sub&&!monitorSubWindow()&&!monitorSelectorWindow()&&boxes[1].contains(px,py)))return false;
  for(int level=2;level>=0;--level){if((level==2&&!monitorSelectorWindow())||(level==1&&(!sub||!monitorSubWindow())))continue;const auto r=boxes[size_t(level)];if(UIRect{r.x,r.y,r.w,22*s}.contains(px,py)){monitorDragLevel=level;monitorDragX=px-r.x;monitorDragY=py-r.y;monitorPicker=false;break;}}}
 if(monitorDragLevel<0)return false;const int level=monitorDragLevel;const auto r=boxes[size_t(level)];const float xx=std::clamp(px-monitorDragX,8*s,std::max(8*s,w-r.w-8*s)),yy=std::clamp(py-monitorDragY,8*s,std::max(8*s,h*.9f-r.h-8*s));
 if(level==0){x=xx/w;y=yy/h;}else{monitorX[size_t(level)]=xx/w;monitorY[size_t(level)]=yy/h;monitorPlaced[size_t(level)]=true;}
 monitorHover=monitorHeaderHover=true;monitorKeyboard=false;monitorHoverLevel=monitorHoverIndex=monitorFooter=-1;return true;
}
