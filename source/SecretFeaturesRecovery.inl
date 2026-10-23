// Classic SA 1.0 US. Only the result recovery update is held; world/camera keep running.
using SecretFeaturesNativeText=void(__cdecl*)();using SecretFeaturesNativeUpdate=void(__cdecl*)();
using SecretFeaturesRestart=void(__cdecl*)(V3,V3*,float*);
SecretFeaturesNativeText originalSecretFeaturesText=nullptr;SecretFeaturesNativeUpdate originalSecretFeaturesRecovery=nullptr;
SecretFeaturesRestart originalSecretFeaturesPolice=nullptr,secretFeaturesHospital=nullptr;
SecretFeaturesRecoveryState secretRecovery;SecretFeaturesRenderer secretRecoveryView;bool secretFeaturesHospitalScope=false,secretRecoveryAvailable=false;
void secretRecoveryReset(){const bool was=secretRecovery.view.active||secretRecovery.resuming;secretRecovery.reset();secretRecoveryView.reset();if(was)audio.endFailure();}
bool secretRecoveryEligible(){const auto gate=openState(focused());return gate.gameState==9&&!gate.loadingData&&!game::loading()&&game::player();}
void __cdecl secretFeaturesPoliceRestart(V3 from,V3*to,float*angle){
 if(secretFeaturesHospitalScope&&secretFeaturesHospital&&to&&angle)secretFeaturesHospital(from,to,angle);
 else originalSecretFeaturesPolice(from,to,angle);
}
// SDK editions disagree about the two restart names. Identify the actual native
// routines by their hospital/police count AND coordinate-table references.
uintptr_t secretFeaturesRestartEntry(uintptr_t entry,uintptr_t count,uintptr_t points){
 uintptr_t resolved=entry,jump=0;if(!qb::readableCode(entry,5))return 0;
 if(game::at<uint8_t>(entry)==0xE9){if(!qb::validateEntryJump(entry,jump))return 0;resolved=jump;}
 std::array<uint8_t,0x1E0> code{};if(!gta::reader(resolved,code.data(),code.size()))return 0;
 auto contains=[&](uintptr_t value){for(size_t n=0;n+4<=code.size();++n){uint32_t v=0;std::memcpy(&v,code.data()+n,4);if(v==value)return true;}return false;};
 return contains(count)&&contains(points)?entry:0;
}
bool secretFeaturesBindRestarts(uintptr_t&police){
 police=0;secretFeaturesHospital=nullptr;
 for(uintptr_t address:{uintptr_t(0x460850),uintptr_t(0x460A50)}){
  if(secretFeaturesRestartEntry(address,0xA4326C,0xA43318))secretFeaturesHospital=reinterpret_cast<SecretFeaturesRestart>(address);
  if(secretFeaturesRestartEntry(address,0xA43268,0xA43390))police=address;
 }
 return police&&secretFeaturesHospital&&police!=reinterpret_cast<uintptr_t>(secretFeaturesHospital);
}
void __cdecl secretFeaturesRecoveryUpdate(){
 const auto now=GetTickCount64();const bool eligible=secretRecoveryEligible();
 const uint8_t status=eligible?game::at<uint8_t>(0x96A8B0):0;const uint32_t event=eligible?game::at<uint32_t>(0x96A8AC):0;
 const bool suspended=!focused()||game::menu()||game::codePaused()||game::userPaused();
 if(secretRecovery.hold(now,status,event,secretFeatures.enabled&&eligible,suspended))return;
 // Tear down ALL result render targets before native streaming/resurrection can
 // reset the graphics device. Never keep a stale player/entity across this call.
 const bool recovering=(status==1||status==2);
 const bool hospital=secretRecovery.resuming&&secretRecovery.matches(status,event)&&secretRecovery.pending==SecretFeaturesChoice::Retry;
 if(recovering){secretFeaturesReleaseGraphics();if(secretRecovery.view.active)secretRecovery.resume();}
 const bool oldScope=secretFeaturesHospitalScope;secretFeaturesHospitalScope=hospital&&status==2;
 originalSecretFeaturesRecovery();secretFeaturesHospitalScope=oldScope;
}
void __cdecl secretFeaturesNativeText(){
 const bool replace=secretRecovery.view.active&&secretRecovery.drawn&&secretFeatures.enabled&&secretRecoveryEligible()&&renderer.uiTextReady()&&focused()&&!game::userPaused()&&!game::menu()&&!game::codePaused();
 if(!replace)originalSecretFeaturesText();
}
void __cdecl secretFeaturesHudDraw(){if(!secretRecovery.resuming)renderer.ensureUiAssets(game::device());secretSchool.script.assetsReady=renderer.uiTextReady();hudDraw();}
bool updateSecretRecovery(uint64_t tick,double dt,bool focus){
 const bool eligible=secretRecoveryEligible();const bool was=secretRecovery.view.active||secretRecovery.resuming;const auto old=secretRecovery.view.serial;
 const uint8_t status=eligible?game::at<uint8_t>(0x96A8B0):0;const uint32_t event=eligible?game::at<uint32_t>(0x96A8AC):0;
 if(!secretRecovery.observe(status,event,secretFeatures.enabled&&secretRecoveryAvailable&&eligible,tick)){
  if(was&&!secretRecovery.resuming){secretRecoveryView.reset();audio.endFailure();blockActions();blockOpenInput();}
  return secretRecovery.resuming;
 }
 if(old!=secretRecovery.view.serial){secretSchoolReset();secretRecoveryView.reset();if(state.open)close();openIntent.clear();blockOpenInput();blockActions();audio.initFailure(gameWindow(),base+L"GT4Showroom/assets/audio/");}
 const bool suspended=!focus||game::menu()||game::codePaused()||game::userPaused();audio.focus(!suspended);
 auto&f=secretRecovery.view;if(suspended){f.suspended();return true;}
 if(secretRecovery.pending!=SecretFeaturesChoice::None){
  secretRecovery.closing+=std::clamp(dt,0.,.1);
  if(secretRecovery.closing>=.18){
   if(secretRecovery.pending==SecretFeaturesChoice::Exit){HWND window=gameWindow();if(window&&PostMessageW(window,WM_CLOSE,0,0)){secretRecovery.quitPosted=true;secretRecovery.resume();secretRecoveryView.reset();audio.endFailure();}else{secretRecovery.pending=SecretFeaturesChoice::None;secretRecovery.closing=0;f.accepted=false;f.suspended();}}
   else{secretRecovery.resume();secretRecoveryView.reset();audio.endFailure();blockActions();blockOpenInput();if(void*p=game::pad())exitButtons.arm(static_cast<int16_t*>(p));}
  }
  return true;
 }
 D3DVIEWPORT9 vp{0,0,1280,720,0,1};if(auto*d=game::device())d->GetViewport(&vp);
 const uintptr_t mouse=0xB73418;float dx=game::at<float>(mouse+0xC),dy=game::at<float>(mouse+0x10);if(game::flag(0xBA6744))dx=-dx;if(game::flag(0xBA6745))dy=-dy;
 SecretFeaturesInput input;input.up=key(VK_UP)||sampledPad.up;input.down=key(VK_DOWN)||sampledPad.down;input.accept=key(VK_RETURN)||key(VK_SPACE)||sampledPad.accept;input.back=key(VK_BACK)||sampledPad.back;input.click=game::at<uint8_t>(mouse)!=0||key(VK_LBUTTON);input.mouseMoved=std::abs(dx)+std::abs(dy)>.03f;input.mouseX=f.pointerX+dx*mouseSensitivity;input.mouseY=f.pointerY+dy*mouseSensitivity;
 const auto choice=f.update(dt,input,float(vp.Width),float(vp.Height));if(!secretRecovery.music&&f.age>=.26){audio.startFailure();secretRecovery.music=true;}
 if(choice!=SecretFeaturesChoice::None){secretRecovery.pending=choice;secretRecovery.closing=0;}
 return true;
}
