// Included in gt4 after native/input/audio services. One ASI, no companion script.
using SchoolGroup=char(__thiscall*)(void*,int);SchoolGroup originalSchool0=nullptr,originalSchool2=nullptr,originalSchool8=nullptr,originalSchool9=nullptr,originalSchool11=nullptr,originalSchool23=nullptr;
using HudDraw=void(__cdecl*)();HudDraw originalHudDraw=nullptr;
#define GT4_SCHOOL_GROUP(N) char __fastcall school##N(void*self,void*,int command){return secretSchool.command(self,command,GetTickCount64())?0:originalSchool##N(self,command);}
GT4_SCHOOL_GROUP(0) GT4_SCHOOL_GROUP(2) GT4_SCHOOL_GROUP(8) GT4_SCHOOL_GROUP(9) GT4_SCHOOL_GROUP(11) GT4_SCHOOL_GROUP(23)
#undef GT4_SCHOOL_GROUP
void secretSchoolReset(){secretSchool.reset();secretSchoolView.reset();secretSchoolSerial=0;secretSchoolMusicPlayed=false;secretSchoolPending=SecretFeaturesChoice::None;secretSchoolClosing=0;audio.endFailure();}
void __cdecl hudDraw(){
 const auto gate=openState(focused());const bool healthy=gate.gameState==9&&gate.alive&&!gate.loadingData;
 if((!healthy||!secretSchool.script.enabled)&&(secretSchool.script.state.active||secretSchool.script.needsScene()))secretSchoolReset();
 originalHudDraw();
}
bool updateSecretSchool(uint64_t tick,double dt,bool focus){
 const bool nativeSuspended=!focus||game::menu()||game::codePaused()||game::userPaused();
 if(nativeSuspended)secretSchool.script.suspended(tick);else secretSchool.script.expire(tick);
 if(!secretSchool.script.enabled||game::loading()||!game::player()||game::field<float>(game::player(),0x540)<=0){if(secretSchoolSerial||secretSchool.script.needsScene())secretSchoolReset();return false;}
 auto&f=secretSchool.script.state;
 if(!f.active){if(secretSchoolSerial){audio.endFailure();secretSchoolView.releaseScene();secretSchoolSerial=0;secretSchoolMusicPlayed=false;secretSchoolPending=SecretFeaturesChoice::None;secretSchoolClosing=0;}return false;}
 if(state.open)close();openIntent.clear();blockOpenInput();
 const bool active=focus&&!game::menu()&&!game::codePaused()&&!game::userPaused();
 if(secretSchoolSerial!=f.serial){secretSchoolSerial=f.serial;secretSchoolMusicPlayed=false;secretSchoolPending=SecretFeaturesChoice::None;secretSchoolClosing=0;audio.initFailure(gameWindow(),base+L"GT4Showroom/assets/audio/");blockActions();}
 audio.focus(active);if(!active){f.suspended();return true;}
 D3DVIEWPORT9 vp{0,0,1280,720,0,1};if(auto*d=game::device())d->GetViewport(&vp);
 if(secretSchoolPending!=SecretFeaturesChoice::None){
  secretSchoolClosing+=std::clamp(dt,0.,.1);
  if(secretSchoolClosing>=.18){const auto choice=secretSchoolPending;if(secretSchool.choose(choice)){secretSchool.script.stop();audio.endFailure();secretSchoolView.releaseScene();secretSchoolSerial=0;secretSchoolMusicPlayed=false;secretSchoolPending=SecretFeaturesChoice::None;secretSchoolClosing=0;returnFade.begin();if(void*p=game::pad())exitButtons.arm(static_cast<int16_t*>(p));blockActions();}else{secretSchoolPending=SecretFeaturesChoice::None;secretSchoolClosing=0;f.accepted=false;f.suspended();log("License choice rejected after live mission revalidation; original state preserved.");}}
  return true;
 }
 const uintptr_t mouse=0xB73418;float dx=game::at<float>(mouse+0xC),dy=game::at<float>(mouse+0x10);if(game::flag(0xBA6744))dx=-dx;if(game::flag(0xBA6745))dy=-dy;
 SecretFeaturesInput input;input.up=key(VK_UP)||sampledPad.up;input.down=key(VK_DOWN)||sampledPad.down;input.accept=key(VK_RETURN)||key(VK_SPACE)||sampledPad.accept;input.back=key(VK_BACK)||sampledPad.back;input.click=game::at<uint8_t>(mouse)!=0||key(VK_LBUTTON);input.mouseMoved=std::abs(dx)+std::abs(dy)>.03f;input.mouseX=f.pointerX+dx*mouseSensitivity;input.mouseY=f.pointerY+dy*mouseSensitivity;
 const int previousSelection=f.selected;const auto choice=f.update(dt,input,float(vp.Width),float(vp.Height));if(f.ready()&&f.selected!=previousSelection)audio.cue(Cue::Move);if(!secretSchoolMusicPlayed&&f.age>=.26){audio.startFailure();secretSchoolMusicPlayed=true;}
 if(choice!=SecretFeaturesChoice::None){audio.cue(choice==SecretFeaturesChoice::Exit?Cue::Back:Cue::Accept);secretSchoolPending=choice;secretSchoolClosing=0;}
 return true;
}
