// Native CHud + CGameLogic bindings verified against the classic SA source/SDK.
using OutcomeText=void(__cdecl*)();using OutcomePrint=void(__cdecl*)(float,float,const char*);
OutcomeText originalOutcomeText=nullptr;OutcomePrint originalOutcomePrint=nullptr;
OutcomeState outcome;LicenseFailRenderer outcomeView;bool suppressOutcomeText=false;
void outcomeReset(){const bool was=outcome.view.active;outcome.reset();outcomeView.reset();if(was)audio.endFailure();}
bool updateOutcome(uint64_t,double dt,bool focus){
 const bool eligible=!game::loading()&&game::player();const bool was=outcome.view.active;const auto old=outcome.view.serial;
 const bool suspended=!focus||game::menu()||game::codePaused()||game::userPaused();
 const uint8_t nativeState=eligible?game::at<uint8_t>(0x96A8B0):0;
 const uint32_t event=eligible?game::at<uint32_t>(0x96A8AC):0;
 if(!outcome.step(nativeState,event,secretFeatures.wastedBusted&&eligible,suspended,dt)){if(was){outcomeView.reset();audio.endFailure();blockActions();blockOpenInput();}return false;}
 if(old!=outcome.view.serial){licenseReset();outcomeView.reset();if(state.open)close();openIntent.clear();blockOpenInput();blockActions();audio.initFailure(gameWindow(),base+L"GT4Showroom/assets/audio/");}
 audio.focus(!suspended);if(!suspended&&!outcome.music&&outcome.view.age>=.26){audio.startFailure();outcome.music=true;}
 return true;
}
// Run the stock message function so its fade/timers still advance. Suppress only
// its own PrintStringFromBottom call, never unrelated text or HUD/radar flags.
void __cdecl outcomeNativeText(){const bool old=suppressOutcomeText;suppressOutcomeText=outcome.view.active&&secretFeatures.wastedBusted&&renderer.uiTextReady()&&focused()&&!game::userPaused()&&!game::loading()&&!game::menu()&&!game::codePaused();originalOutcomeText();suppressOutcomeText=old;}
void __cdecl outcomePrint(float x,float y,const char*text){if(suppressOutcomeText&&text==reinterpret_cast<const char*>(0xBAACC0+2*128))return;originalOutcomePrint(x,y,text);}
void __cdecl outcomeHudDraw(){
 const bool focus=focused();const auto gate=openState(focus);if(auto*d=game::device()){
  renderer.ensureUiAssets(d);licenseNative.script.assetsReady=renderer.uiTextReady();
  if(outcome.view.active)outcomeView.recorder.capture(d,GetTickCount64(),outcome.view.serial,gate.gameState==9&&!gate.loadingData&&focus&&!gate.menu&&!gate.codePause&&!gate.userPause,true);
 }
 hudDraw();
}
