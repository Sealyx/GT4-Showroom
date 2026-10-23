// Included in gt4 after native/input/audio services. One ASI, no companion script.
using SchoolGroup=char(__thiscall*)(void*,int);SchoolGroup originalSchool0=nullptr,originalSchool2=nullptr,originalSchool8=nullptr,originalSchool9=nullptr,originalSchool11=nullptr,originalSchool23=nullptr;
using HudDraw=void(__cdecl*)();HudDraw originalHudDraw=nullptr;
#define GT4_SCHOOL_GROUP(N) char __fastcall school##N(void*self,void*,int command){return licenseNative.command(self,command,GetTickCount64())?0:originalSchool##N(self,command);}
GT4_SCHOOL_GROUP(0) GT4_SCHOOL_GROUP(2) GT4_SCHOOL_GROUP(8) GT4_SCHOOL_GROUP(9) GT4_SCHOOL_GROUP(11) GT4_SCHOOL_GROUP(23)
#undef GT4_SCHOOL_GROUP
void licenseReset(){licenseNative.reset();licenseView.reset();licenseSerial=0;licenseMusicPlayed=false;licensePending=LicenseFailChoice::None;licenseClosing=0;audio.endFailure();}
void __cdecl hudDraw(){
 const auto tick=GetTickCount64();const auto gate=openState(focused());const bool healthy=gate.gameState==9&&gate.alive&&!gate.loadingData;
 if((!healthy||!licenseNative.script.enabled)&&(licenseNative.script.state.active||licenseNative.script.needsReplay()))licenseReset();
 if(auto*d=game::device())licenseView.recorder.capture(d,tick,licenseNative.script.recordSerial,healthy&&gate.focus&&!gate.menu&&!gate.codePause&&!gate.userPause&&!state.open&&licenseNative.script.recording,licenseNative.script.needsReplay());
 originalHudDraw();
}
bool updateLicense(uint64_t tick,double dt,bool focus){
 const bool nativeSuspended=!focus||game::menu()||game::codePaused()||game::userPaused();
 if(nativeSuspended)licenseNative.script.suspended(tick);else licenseNative.script.expire(tick);
 if(!licenseNative.script.enabled||game::loading()||!game::player()||game::field<float>(game::player(),0x540)<=0){if(licenseSerial||licenseNative.script.needsReplay())licenseReset();return false;}
 auto&f=licenseNative.script.state;
 if(!f.active){if(licenseSerial){audio.endFailure();licenseView.releaseReplay();licenseView.recorder.reset();licenseSerial=0;licenseMusicPlayed=false;licensePending=LicenseFailChoice::None;licenseClosing=0;}return false;}
 if(state.open)close();openIntent.clear();blockOpenInput();
 const bool active=focus&&!game::menu()&&!game::codePaused()&&!game::userPaused();
 if(licenseSerial!=f.serial){licenseSerial=f.serial;licenseMusicPlayed=false;licensePending=LicenseFailChoice::None;licenseClosing=0;audio.initFailure(gameWindow(),base+L"GT4Showroom/assets/audio/");blockActions();}
 audio.focus(active);if(!active){f.suspended();return true;}
 D3DVIEWPORT9 vp{0,0,1280,720,0,1};if(auto*d=game::device())d->GetViewport(&vp);
 if(licensePending!=LicenseFailChoice::None){
  licenseClosing+=std::clamp(dt,0.,.1);
  if(licenseClosing>=.18){const auto choice=licensePending;if(licenseNative.choose(choice)){audio.cue(choice==LicenseFailChoice::Exit?Cue::Back:Cue::Accept);licenseNative.script.stop();audio.endFailure();licenseView.releaseReplay();licenseView.recorder.reset();licenseSerial=0;licenseMusicPlayed=false;licensePending=LicenseFailChoice::None;licenseClosing=0;returnFade.begin();if(void*p=game::pad())exitButtons.arm(static_cast<int16_t*>(p));blockActions();}else{licensePending=LicenseFailChoice::None;licenseClosing=0;f.accepted=false;f.suspended();log("License choice rejected after live mission revalidation; original state preserved.");}}
  return true;
 }
 const uintptr_t mouse=0xB73418;float dx=game::at<float>(mouse+0xC),dy=game::at<float>(mouse+0x10);if(game::flag(0xBA6744))dx=-dx;if(game::flag(0xBA6745))dy=-dy;
 LicenseFailInput input;input.up=key(VK_UP)||sampledPad.up;input.down=key(VK_DOWN)||sampledPad.down;input.accept=key(VK_RETURN)||key(VK_SPACE)||sampledPad.accept;input.back=key(VK_BACK)||sampledPad.back;input.click=game::at<uint8_t>(mouse)!=0||key(VK_LBUTTON);input.mouseMoved=std::abs(dx)+std::abs(dy)>.03f;input.mouseX=f.pointerX+dx*mouseSensitivity;input.mouseY=f.pointerY+dy*mouseSensitivity;
 const auto choice=f.update(dt,input,float(vp.Width),float(vp.Height));if(!licenseMusicPlayed&&f.age>=.26){audio.startFailure();licenseMusicPlayed=true;}
 if(choice!=LicenseFailChoice::None){licensePending=choice;licenseClosing=0;}
 return true;
}
