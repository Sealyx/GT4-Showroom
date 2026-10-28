
using SecretFeaturesNativeText=void(__cdecl*)();using SecretFeaturesNativeUpdate=void(__cdecl*)();
SecretFeaturesNativeText originalSecretFeaturesText=nullptr;SecretFeaturesNativeUpdate originalSecretFeaturesRecovery=nullptr;
SecretFeaturesRecoveryState secretRecovery;SecretFeaturesRenderer secretRecoveryView;RecoveryFade secretRetryFade;bool secretRecoveryAvailable=false;
void secretRecoveryReset(){outcomeCamera.stop();secretRetryFade.reset();const bool was=secretRecovery.view.active||secretRecovery.resuming;secretRecovery.reset();secretRecoveryView.reset();if(was)audio.endFailure();}
bool secretRecoveryEligible(){const auto gate=openState(focused());return gate.gameState==9&&!gate.loadingData&&!game::loading()&&game::player();}
void __cdecl secretFeaturesRecoveryUpdate(){
 const auto now=GetTickCount64();const bool eligible=secretRecoveryEligible();
 const uint8_t status=eligible?game::at<uint8_t>(0x96A8B0):0;const uint32_t event=eligible?game::at<uint32_t>(0x96A8AC):0;
 const bool suspended=!focused()||game::menu()||game::codePaused()||game::userPaused();
 if(secretRetryFade.waiting()&&secretFeatures.enabled&&eligible&&secretRecovery.view.active)return;
 if(secretRecovery.hold(now,status,event,secretFeatures.enabled&&eligible,suspended))return;
 const bool recovering=(status==1||status==2);
 if(recovering){outcomeCamera.stop();secretFeaturesReleaseGraphics();if(secretRecovery.view.active)secretRecovery.resume();}
 originalSecretFeaturesRecovery();
}
void __cdecl secretFeaturesNativeText(){
 const bool fading=secretRetryFade.active()&&secretFeatures.enabled&&secretRecoveryEligible()&&focused()&&!game::userPaused()&&!game::menu()&&!game::codePaused();
 const bool replace=fading||(secretRecovery.view.active&&secretRecovery.drawn&&secretFeatures.enabled&&secretRecoveryEligible()&&renderer.uiTextReady()&&focused()&&!game::userPaused()&&!game::menu()&&!game::codePaused());
 if(!replace)originalSecretFeaturesText();
}
void __cdecl secretFeaturesHudDraw(){if(!secretRecovery.resuming)renderer.ensureUiAssets(game::device());secretSchool.script.assetsReady=renderer.uiTextReady();hudDraw();}
bool updateSecretRecovery(uint64_t tick,double dt,bool focus){
 const bool eligible=secretRecoveryEligible();const bool was=secretRecovery.view.active||secretRecovery.resuming;const bool newEvent=!secretRecovery.matches(eligible?game::at<uint8_t>(0x96A8B0):0,eligible?game::at<uint32_t>(0x96A8AC):0)||!was;
 const uint8_t status=eligible?game::at<uint8_t>(0x96A8B0):0;const uint32_t event=eligible?game::at<uint32_t>(0x96A8AC):0;
 const bool transitionSuspended=!focus||game::menu()||game::codePaused()||game::userPaused();
 if(secretRetryFade.active()){
  if(!secretFeatures.enabled||game::loading()){secretRetryFade.reset();}
  else{secretRetryFade.step(dt,transitionSuspended,secretRecovery.resuming&&status==0&&secretFeaturesRecoverySceneReady());if(secretRecovery.resuming){audio.focus(!transitionSuspended);if(secretRetryFade.active())return true;secretRecovery.reset();blockActions();blockOpenInput();if(void*p=game::pad())exitButtons.arm(static_cast<int16_t*>(p));return false;}}
 }
 if(!secretRecovery.observe(status,event,secretFeatures.enabled&&secretRecoveryAvailable&&eligible,tick)){
  if(was&&!secretRecovery.resuming){outcomeCamera.stop();secretRecoveryView.reset();audio.endFailure();blockActions();blockOpenInput();}
  return secretRecovery.resuming;
 }
 if(newEvent){secretSchoolReset();secretRecoveryView.reset();if(state.open)close();openIntent.clear();blockOpenInput();blockActions();audio.initFailure(gameWindow(),base+L"GT4Showroom/assets/audio/");const bool camera=outcomeCamera.begin(status==2);log(camera?"Outcome camera started with validated player/culprit references.":"Outcome camera unavailable; existing camera retained.");}
 const bool suspended=!focus||game::menu()||game::codePaused()||game::userPaused();audio.focus(!suspended);
 outcomeCamera.update(dt,suspended);auto&f=secretRecovery.view;if(suspended){f.suspended();return true;}
 if(secretRecovery.pending==SecretFeaturesChoice::Retry){
  if(secretRetryFade.readyToResume()){outcomeCamera.stop();secretRetryFade.markNativeResume(secretFeaturesWorldGeneration());secretRecovery.resume();secretRecoveryView.reset();audio.endFailure();blockActions();blockOpenInput();if(void*p=game::pad())exitButtons.arm(static_cast<int16_t*>(p));log("Retry: black hold complete; native hospital/police recovery released.");}
  return true;
 }
 if(secretRecovery.pending==SecretFeaturesChoice::Exit){
  secretRecovery.closing+=std::clamp(dt,0.,.1);
  if(secretRecovery.closing>=.18){HWND window=gameWindow();if(window&&PostMessageW(window,WM_CLOSE,0,0)){secretRecovery.quitPosted=true;outcomeCamera.stop();secretRecovery.resume();secretRecoveryView.reset();audio.endFailure();}else{secretRecovery.pending=SecretFeaturesChoice::None;secretRecovery.closing=0;f.accepted=false;f.suspended();}}
  return true;
 }
 D3DVIEWPORT9 vp{0,0,1280,720,0,1};if(auto*d=game::device())d->GetViewport(&vp);
 const uintptr_t mouse=0xB73418;float dx=game::at<float>(mouse+0xC),dy=game::at<float>(mouse+0x10);if(game::flag(0xBA6744))dx=-dx;if(game::flag(0xBA6745))dy=-dy;
 SecretFeaturesInput input;input.up=key(VK_UP)||sampledPad.up;input.down=key(VK_DOWN)||sampledPad.down;input.accept=key(VK_RETURN)||key(VK_SPACE)||sampledPad.accept;input.back=key(VK_BACK)||sampledPad.back;input.click=game::at<uint8_t>(mouse)!=0||key(VK_LBUTTON);input.mouseMoved=std::abs(dx)+std::abs(dy)>.03f;input.mouseX=f.pointerX+dx*mouseSensitivity;input.mouseY=f.pointerY+dy*mouseSensitivity;
 const int previousSelection=f.selected;const auto choice=f.update(dt,input,float(vp.Width),float(vp.Height));if(f.ready()&&f.selected!=previousSelection)audio.cue(Cue::Move);if(!secretRecovery.music&&f.age>=.26){audio.startFailure();secretRecovery.music=true;}
 if(choice!=SecretFeaturesChoice::None){audio.cue(choice==SecretFeaturesChoice::Exit?Cue::Back:Cue::Accept);secretRecovery.pending=choice;secretRecovery.closing=0;if(choice==SecretFeaturesChoice::Retry){secretRetryFade.begin();log("Retry: fading out; holding full black for at least 0.6 seconds before native recovery.");}}
 return true;
}
