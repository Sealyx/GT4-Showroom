inline Key previousCinematicHotspot,nextCinematicHotspot,pauseCinematic;
void finishSettings(){
 auto&ui=state.ui;if(!ui.menuSnapshot||ui.menu.open)return;
 if(ui.menu.closeResult<0){ui.cinematic=ui.menuCinematic;ui.vehicle=ui.menuVehicle;ui.audioLevels=ui.menuAudio;}
 else if(ui.audioLevels!=ui.menuAudio)saveStudioAudioPreferences();
 audio.setLevels(ui.audioLevels);ui.menuSnapshot=false;
}
void requestStudioAction(StudioAction action){
 if(action==StudioAction::None||state.busy())return;
 if(action==StudioAction::Shadows||action==StudioAction::CarLogo||action==StudioAction::BrandLogo||action==StudioAction::DisplayModelId||action==StudioAction::DisplayModelName){const bool value=state.ui.menu.choice!=0;if(action==StudioAction::Shadows)state.ui.shadows=value;else if(action==StudioAction::CarLogo)state.ui.carLogo=value;else if(action==StudioAction::BrandLogo)state.ui.brandLogo=value;else if(action==StudioAction::DisplayModelId)state.ui.displayModelId=value;else state.ui.displayModelName=value;state.ui.menu.open=true;state.ui.menu.selectedAction();state.cue=Cue::Accept;blockActions();return;}
 if(action==StudioAction::Extra1||action==StudioAction::Extra2){const bool applied=selectStudioExtra(action==StudioAction::Extra2?1:0,state.ui.menu.choice);state.ui.menu.open=true;state.ui.menu.selectedAction();state.cue=applied?Cue::Accept:Cue::Error;blockActions();return;}
 if(action==StudioAction::Hotspots||action==StudioAction::Lights||action==StudioAction::Indicators){auto&v=state.ui.vehicle;const int value=state.ui.menu.choice;if(action==StudioAction::Hotspots)v.hotspots=value!=0;else if(action==StudioAction::Lights)v.lights=value!=0;else v.indicators=IndicatorMode(std::clamp(value,0,3));state.ui.menu.open=true;state.ui.menu.selectedAction();state.cue=Cue::Accept;blockActions();return;}
 if(action==StudioAction::Cinematic){if(entries.empty()||state.page==Page::Prize)return;auto&c=state.ui.cinematic;const bool enabled=state.ui.menu.choice!=0;if(!enabled)c.leave();else if(!c.active)c.enter(state.clock,state.enthusia?enthusiaPose(state.clock-state.motionBegan).yaw:state.heading()+pi);state.ui.menu.open=true;state.ui.menu.cinematicView=c.active;state.ui.dock=true;state.ui.pointerMoved=state.clock;state.cue=Cue::Accept;blockActions();return;}
 state.ui.menu.open=false;state.ui.transition.request(action,state.clock);state.dragging=false;state.cue=Cue::Accept;
}
void applyViewTransition(){
 auto&t=state.ui.transition;if(!t.ready())return;const auto action=t.action;const bool returnPointer=state.ui.dock;
 if(action>=StudioAction::SortAscending&&action<=StudioAction::SortModelId){state.ui.sortMode=int(action)-int(StudioAction::SortAscending);studioConfig.sort=menuSort(state.ui.sortMode);}
 else if(action==StudioAction::FilterCountry)state.ui.countryFilter=state.ui.menu.choice;else if(action==StudioAction::FilterClass)state.ui.classFilter=state.ui.menu.choice;
 else if(action==StudioAction::Favorites)state.ui.favoritesOnly=true;else if(action==StudioAction::AllCars)state.ui.favoritesOnly=false;
 state.leavePrize();state.ui.cinematic.leave();state.page=Page::Cars;state.paint=state.livery=0;state.paintMix=0;state.spawn=false;state.searching=false;state.ui.keyboard.open=false;rebuildCatalogueView();refreshSelected();state.ui.dock=returnPointer;state.changed=state.motionBegan=state.clock-2;state.status.clear();pending.clear();t.applied(state.clock);blockActions();
}
void favoriteSelection(bool add){
 if(entries.empty()||state.selected>=entries.size()||state.page!=Page::Cars||state.busy())return;auto&e=entries[state.selected];if(e.favorite==add)return;e.favorite=add;
 for(auto&c:catalog)if(c.id==e.id)c.favorite=add;
 WritePrivateProfileStringW(L"Favorites",std::to_wstring(e.id).c_str(),add?L"1":L"0",(base+L"GT4Showroom\\favorites.ini").c_str());state.cue=Cue::Favorite;
 refreshCountryMenu();
 if(!add&&state.ui.favoritesOnly)requestStudioAction(StudioAction::RefreshFavorites);
}
void submitStudioSearch(){state.submitSearch(entries);state.ui.dock=true;state.ui.keyboard.open=false;if(state.page==Page::Prize)audio.endPrize(false);blockActions();}
void interaction(const PadActions&pad,float W,float H,double dt,uint64_t tick){
 if(!entries.empty()&&state.selected<entries.size())state.ui.vehicle.bind(entries[state.selected].id);
 applyViewTransition();state.ui.menu.sync(state.ui.favoritesOnly,state.ui.cinematic.active,state.ui.sortMode,state.ui.countryFilter,state.ui.classFilter,state.ui.shadows,state.ui.carLogo,state.ui.brandLogo,state.ui.displayModelId,state.ui.displayModelName);
 state.ui.menu.hotspots=state.ui.vehicle.hotspots;state.ui.menu.lights=state.ui.vehicle.lights;state.ui.menu.indicators=int(state.ui.vehicle.indicators);
 const uintptr_t rawMouse=0xB73418;float dx=game::at<float>(rawMouse+0xC),dy=game::at<float>(rawMouse+0x10);if(game::flag(0xBA6744))dx=-dx;if(game::flag(0xBA6745))dy=-dy;
 const bool lDown=game::at<uint8_t>(rawMouse)!=0||key(VK_LBUTTON),rDown=game::at<uint8_t>(rawMouse+1)!=0||key(VK_RBUTTON);
 state.pointerMove(dx*mouseSensitivity,dy*mouseSensitivity,W,H);
 const bool click=mouseLeft.poll(lDown,tick),context=mouseRight.poll(rDown||pad.view||key(VK_APPS),tick);
 const bool search=searchKey.poll(key(VK_OEM_3)||pad.search,tick);
 const bool leftPress=left.poll(key(VK_LEFT)||pad.left,tick,true),rightPress=right.poll(key(VK_RIGHT)||pad.right,tick,true);
 const bool modal=state.ui.menu.open||state.searching;
 const bool upPress=up.poll(key(VK_UP)||pad.up,tick,modal),downPress=down.poll(key(VK_DOWN)||pad.down,tick,modal);
 const bool accept=enter.poll(key(VK_RETURN)||key(VK_SPACE)||pad.accept,tick),goBack=backspace.poll(key(VK_BACK)||pad.back,tick);
 const bool add=addFavorite.poll(pad.add,tick),favoritesPress=favorite.poll((!state.searching&&key('F'))||pad.favorites,tick);
 const bool previousHotspot=previousCinematicHotspot.poll(pad.previousHotspot||(key(VK_TAB)&&key(VK_SHIFT)),tick),nextHotspot=nextCinematicHotspot.poll(pad.nextHotspot||(key(VK_TAB)&&!key(VK_SHIFT)),tick),pauseView=pauseCinematic.poll(key('P'),tick);
 if(pad.accept||pad.back||pad.add||pad.favorites||pad.view||pad.search||pad.left||pad.right||pad.up||pad.down||pad.previousHotspot||pad.nextHotspot||pad.orbitX||pad.orbitY||pad.zoom){state.ui.controllerUsed=true;}
 if(lDown||rDown||key(VK_APPS)||key(VK_LEFT)||key(VK_RIGHT)||key(VK_UP)||key(VK_DOWN)||key(VK_RETURN)||key(VK_SPACE)||key(VK_BACK)||key(VK_TAB)||key('P')||key('F')||key(VK_OEM_3))state.ui.controllerUsed=false;
 if(leftPress||rightPress||upPress||downPress||accept||goBack||search||favoritesPress||add)state.ui.dock=true;
 if(state.busy())return;
 if(search){if(state.ui.menu.open){state.ui.menu.finish(-1);finishSettings();}if(state.searching){state.searching=false;state.ui.keyboard.open=false;}else{state.beginSearch();state.ui.keyboard.open=pad.search;state.ui.keyboard.selected=0;}blockActions();return;}
 if(state.searching){
  if(state.ui.keyboard.open){auto&kb=state.ui.keyboard;if(leftPress)kb.move(-1,0);if(rightPress)kb.move(1,0);if(upPress)kb.move(0,-1);if(downPress)kb.move(0,1);
   int action=0;if(accept&&pad.accept)action=kb.type(state.query);if(add&&!state.query.empty())state.query.pop_back();if(favoritesPress)action=1;if(goBack&&pad.back)action=-1;
   if(click)for(int i=0;i<40;++i)if(kb.box(W,H,i).contains(state.pointerX*W,state.pointerY*H)){kb.selected=i;action=kb.type(state.query);break;}
   if(action==1){submitStudioSearch();return;}if(action<0){state.searching=false;kb.open=false;blockActions();return;}
  }else if(goBack&&pad.back){state.searching=false;blockActions();return;}
  const auto action=searchInput.poll([](int v){return key(v);},tick,state.query);
  if(action==SearchAction::Submit)submitStudioSearch();else if(action==SearchAction::Cancel){state.searching=false;state.ui.keyboard.open=false;blockActions();}dockPointer(state,entries,W,H,dt);return;
 }
 if(context&&state.page!=Page::Prize){auto&ui=state.ui;const bool opened=!ui.menu.open;if(opened){ui.menuCinematic=ui.cinematic;ui.menuVehicle=ui.vehicle;ui.menuAudio=ui.audioLevels;ui.menuSnapshot=true;ui.menu.volumes=ui.audioLevels;ui.menu.toggle(state.pointerX,state.pointerY,state.clock);}else{ui.menu.monitorDragLevel=ui.menu.sliderDrag=-1;ui.menu.monitorBack();}ui.menu.monitorKeyboard=pad.view||key(VK_APPS);state.cue=opened?Cue::Open:Cue::Back;state.dragging=false;ui.cinematic.grabbed=false;ui.vehicle.endWheelDrag();if(pad.view||key(VK_APPS))ui.dock=true;finishSettings();blockActions();dockPointer(state,entries,W,H,dt);return;}
 if(state.ui.menu.open){auto&m=state.ui.menu;const int previousTarget=m.audibleTarget();const auto previous=std::make_tuple(m.open,m.sub,m.focus(),m.monitorPicker);
  if(goBack){m.monitorDragLevel=m.sliderDrag=-1;m.monitorBack();finishSettings();state.cue=Cue::Back;blockActions();return;}
  auto applySliders=[&]{if(!m.sliderChanged)return;if(m.childBranch()==MenuBranch::Volume){state.ui.audioLevels=m.volumes;audio.setLevels(m.volumes);}else if(m.childBranch()==MenuBranch::Extras){const int group=m.extraGroup(m.sortRow);if(group>=0&&!selectStudioExtra(group,m.extraChoice[size_t(group)]))state.cue=Cue::Error;}m.sliderChanged=false;if(state.cue==Cue::None)state.cue=Cue::Move;};
  if(m.monitorSlide(state.pointerX*W,state.pointerY*H,W,H,lDown,click)){applySliders();state.ui.dock=false;return;}
  if(m.monitorDrag(state.pointerX*W,state.pointerY*H,W,H,lDown,click)){state.ui.dock=false;return;}
  if(std::abs(dx)+std::abs(dy)>.03f)m.hover(state.pointerX*W,state.pointerY*H,W,H);if(upPress)m.move(0,-1);if(downPress)m.move(0,1);if(leftPress)m.move(-1,0);if(rightPress)m.move(1,0);
  if(game::at<uint8_t>(rawMouse+3))m.move(0,-1);else if(game::at<uint8_t>(rawMouse+4))m.move(0,1);
  applySliders();
  if(accept){m.monitorKeyboard=true;requestStudioAction(m.activate(state.ui.favoritesOnly));}if(click)requestStudioAction(m.click(state.pointerX*W,state.pointerY*H,W,H,state.ui.favoritesOnly));
  const auto current=std::make_tuple(m.open,m.sub,m.focus(),m.monitorPicker);
  const int target=m.audibleTarget();if(state.cue==Cue::None){if(previous!=current&&(accept||click||rightPress))state.cue=Cue::Accept;else if(target>=0&&target!=previousTarget)state.cue=Cue::Move;}
  if(previous!=current)m.monitorChanged=state.clock;
  finishSettings();
  dockPointer(state,entries,W,H,dt);return;
 }

 if(state.ui.cinematic.active){auto&c=state.ui.cinematic;auto&v=state.ui.vehicle;
  if(goBack){v.endWheelDrag();state.back();blockActions();return;}
  const Mesh*mesh=entries.empty()?nullptr:lookup(entries[state.selected].id);
  const bool hasHotspots=mesh&&v.hotspots&&mesh->cinematic.any();
  if(hasHotspots&&(previousHotspot||nextHotspot)){const auto points=renderer.hotspots(*mesh,v);v.focus=points.cycle(v.focus,previousHotspot?-1:1);state.ui.dock=true;state.ui.pointerMoved=state.clock;state.cue=Cue::Move;}
  const bool wheelFocused=hasHotspots&&v.focus>=6&&mesh->cinematic.supported(v.focus);
  if(accept){if(hasHotspots&&v.focus>=0){if(v.activate(v.focus,mesh->cinematic)){state.ui.pointerMoved=state.clock;state.cue=Cue::Accept;}}else c.toggle(state.clock);}if(pauseView)c.toggle(state.clock);
  bool hotspotClicked=false;if(click&&hasHotspots){const int hit=renderer.hitHotspot(*mesh,v,state.pointerX*W,state.pointerY*H);if(hit>=0){hotspotClicked=hit>=6?v.beginWheelDrag(hit,mesh->cinematic,state.pointerX*W):v.activate(hit,mesh->cinematic);state.ui.pointerMoved=state.clock;state.cue=hotspotClicked?Cue::Accept:Cue::Error;}}
  if(!lDown||!hasHotspots)v.endWheelDrag();
  if(v.wheelDrag>=0){v.dragWheel(state.pointerX*W,W);c.grabbed=false;state.ui.dock=false;state.ui.pointerMoved=state.clock;return;}
  if(click&&!hotspotClicked&&mesh&&renderer.hitCar(*mesh,state.pointerX*W,state.pointerY*H)){c.toggle(state.clock);c.grabbed=true;c.dragged=false;c.grabX=state.pointerX*W;c.grabY=state.pointerY*H;}
  if(!lDown)c.grabbed=false;
  if(c.grabbed){const float px=state.pointerX*W,py=state.pointerY*H;if(c.dragged||std::abs(px-c.grabX)+std::abs(py-c.grabY)>3){c.orbit(dx*.007f,dy*.004f,state.clock);c.dragged=true;}}
  if(wheelFocused){const float direction=(key(VK_RIGHT)||pad.right?1.f:0.f)-(key(VK_LEFT)||pad.left?1.f:0.f)+pad.orbitX;v.steer(direction,dt);if(direction){state.ui.pointerMoved=state.clock;state.ui.dock=true;}}else{c.orbit(pad.orbitX*float(dt)*1.8f,pad.orbitY*float(dt)*.9f,state.clock);if(leftPress)c.orbit(-.09f,0,state.clock);if(rightPress)c.orbit(.09f,0,state.clock);}if(upPress)c.orbit(0,-.06f,state.clock);if(downPress)c.orbit(0,.06f,state.clock);
  if(game::at<uint8_t>(rawMouse+3))c.wheel(1);else if(game::at<uint8_t>(rawMouse+4))c.wheel(-1);if(pad.zoom&&!(hasHotspots&&pad.zoomFromShoulders))c.wheel(pad.zoom*float(dt)*8);
  if(hasHotspots&&state.ui.dock&&v.focus>=0){const auto points=renderer.hotspots(*mesh,v);for(size_t i=0;i<points.count;++i)if(points.items[i].component==v.focus){const float a=float(1-std::exp(-std::max(0.,dt)*14));state.pointerX+=(points.items[i].x/W-state.pointerX)*a;state.pointerY+=(points.items[i].y/H-state.pointerY)*a;break;}}
  return;
 }
 if(leftPress)state.move(-1,entries);if(rightPress)state.move(1,entries);if(accept)state.accept(entries);if(goBack)state.back();
 if(favoritesPress&&state.page!=Page::Prize)requestStudioAction(state.ui.favoritesOnly?StudioAction::AllCars:StudioAction::Favorites);
 if(upPress||add)favoriteSelection(true);if(downPress)favoriteSelection(false);
 pointerButton(state,entries,W,H,lDown,click);
 if(!state.dragging){if(game::at<uint8_t>(rawMouse+3))state.move(-1,entries);else if(game::at<uint8_t>(rawMouse+4))state.move(1,entries);}
 dockPointer(state,entries,W,H,dt);
}
