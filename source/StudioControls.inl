// Included inside gt4 after native services/globals. No separate runtime bridge.
void requestStudioAction(StudioAction action){
 if(action==StudioAction::None||state.busy())return;
 if(action==StudioAction::Shadows||action==StudioAction::CarLogo||action==StudioAction::BrandLogo){const bool value=state.ui.menu.choice!=0;if(action==StudioAction::Shadows)state.ui.shadows=value;else if(action==StudioAction::CarLogo)state.ui.carLogo=value;else state.ui.brandLogo=value;state.ui.menu.open=false;state.cue=Cue::Accept;blockActions();return;}
 if(action==StudioAction::Extra1||action==StudioAction::Extra2){const bool applied=selectStudioExtra(action==StudioAction::Extra2?1:0,state.ui.menu.choice);state.ui.menu.open=false;state.cue=applied?Cue::Accept:Cue::Error;blockActions();return;}
 if(action==StudioAction::ReloadSettings){state.ui.menu.open=false;reloadAll();state.cue=Cue::Accept;blockActions();return;}
 if(action==StudioAction::Cinematic){if(entries.empty()||state.page==Page::Prize)return;auto&c=state.ui.cinematic;if(c.active)c.leave();else c.enter(state.clock,state.enthusia?enthusiaPose(state.clock-state.motionBegan).yaw:state.heading()+pi);state.ui.menu.open=false;state.ui.dock=false;state.ui.pointerMoved=state.clock;state.cue=Cue::Accept;return;}
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
 applyViewTransition();state.ui.menu.sync(state.ui.favoritesOnly,state.ui.cinematic.active,state.ui.sortMode,state.ui.countryFilter,state.ui.classFilter,state.ui.shadows,state.ui.carLogo,state.ui.brandLogo);
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
 if(pad.accept||pad.back||pad.add||pad.favorites||pad.view||pad.search||pad.left||pad.right||pad.up||pad.down||pad.orbitX||pad.orbitY||pad.zoom){state.ui.controllerUsed=true;}
 if(leftPress||rightPress||upPress||downPress||accept||goBack||search||favoritesPress||add)state.ui.dock=true;
 if(state.busy())return;
 if(search){if(state.searching){state.searching=false;state.ui.keyboard.open=false;}else{state.beginSearch();state.ui.keyboard.open=pad.search;state.ui.keyboard.selected=0;}blockActions();return;}
 if(state.searching){
  if(state.ui.keyboard.open){auto&kb=state.ui.keyboard;if(leftPress)kb.move(-1,0);if(rightPress)kb.move(1,0);if(upPress)kb.move(0,-1);if(downPress)kb.move(0,1);
   int action=0;if(accept&&pad.accept)action=kb.type(state.query);if(add&&!state.query.empty())state.query.pop_back();if(favoritesPress)action=1;if(goBack&&pad.back)action=-1;
   if(click)for(int i=0;i<40;++i)if(kb.box(W,H,i).contains(state.pointerX*W,state.pointerY*H)){kb.selected=i;action=kb.type(state.query);break;}
   if(action==1){submitStudioSearch();return;}if(action<0){state.searching=false;kb.open=false;blockActions();return;}
  }else if(goBack&&pad.back){state.searching=false;blockActions();return;}
  const auto action=searchInput.poll([](int v){return key(v);},tick,state.query);
  if(action==SearchAction::Submit)submitStudioSearch();else if(action==SearchAction::Cancel){state.searching=false;state.ui.keyboard.open=false;blockActions();}dockPointer(state,entries,W,H,dt);return;
 }
 if(context&&state.page!=Page::Prize){state.ui.menu.toggle(state.pointerX,state.pointerY,state.clock);state.dragging=false;state.ui.cinematic.grabbed=false;if(pad.view||key(VK_APPS))state.ui.dock=true;dockPointer(state,entries,W,H,dt);return;}
 if(state.ui.menu.open){auto&m=state.ui.menu;if(std::abs(dx)+std::abs(dy)>.03f)m.hover(state.pointerX*W,state.pointerY*H,W,H);if(upPress)m.move(0,-1);if(downPress)m.move(0,1);if(leftPress)m.move(-1,0);if(rightPress)m.move(1,0);
  if(game::at<uint8_t>(rawMouse+3))m.move(0,-1);else if(game::at<uint8_t>(rawMouse+4))m.move(0,1);
  if(goBack){if(m.focus()>0)m.move(-1,0);else m.open=false;blockActions();return;}if(accept)requestStudioAction(m.activate(state.ui.favoritesOnly));if(click)requestStudioAction(m.click(state.pointerX*W,state.pointerY*H,W,H,state.ui.favoritesOnly));dockPointer(state,entries,W,H,dt);return;
 }
 if(state.ui.cinematic.active){auto&c=state.ui.cinematic;
  if(goBack){state.back();blockActions();return;}if(accept)c.toggle(state.clock);
  const Mesh*mesh=entries.empty()?nullptr:lookup(entries[state.selected].id);
  if(click&&mesh&&renderer.hitCar(*mesh,state.pointerX*W,state.pointerY*H)){c.toggle(state.clock);c.grabbed=true;c.dragged=false;c.grabX=state.pointerX*W;c.grabY=state.pointerY*H;}
  if(!lDown)c.grabbed=false;
  if(c.grabbed){const float px=state.pointerX*W,py=state.pointerY*H;if(c.dragged||std::abs(px-c.grabX)+std::abs(py-c.grabY)>3){c.orbit(dx*.007f,dy*.004f,state.clock);c.dragged=true;}}
  c.orbit(pad.orbitX*float(dt)*1.8f,pad.orbitY*float(dt)*.9f,state.clock);if(leftPress)c.orbit(-.09f,0,state.clock);if(rightPress)c.orbit(.09f,0,state.clock);if(upPress)c.orbit(0,-.06f,state.clock);if(downPress)c.orbit(0,.06f,state.clock);
  if(game::at<uint8_t>(rawMouse+3))c.wheel(1);else if(game::at<uint8_t>(rawMouse+4))c.wheel(-1);if(pad.zoom)c.wheel(pad.zoom*float(dt)*8);return;
 }
 if(leftPress)state.move(-1,entries);if(rightPress)state.move(1,entries);if(accept)state.accept(entries);if(goBack)state.back();
 if(favoritesPress&&state.page!=Page::Prize)requestStudioAction(state.ui.favoritesOnly?StudioAction::AllCars:StudioAction::Favorites);
 if(upPress||add)favoriteSelection(true);if(downPress)favoriteSelection(false);
 pointerButton(state,entries,W,H,lDown,click);
 if(!state.dragging){if(game::at<uint8_t>(rawMouse+3))state.move(-1,entries);else if(game::at<uint8_t>(rawMouse+4))state.move(1,entries);}
 dockPointer(state,entries,W,H,dt);
}
