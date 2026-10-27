#pragma once
#include "AudioLevels.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "Showroom.hpp"
#include <mmsystem.h>
#include <dsound.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
namespace gt4 {
struct PcmClip {WAVEFORMATEX format{};std::vector<char> bytes;};
 
inline bool readPcm(const std::filesystem::path& name,size_t limit,PcmClip& clip){
 clip={};std::ifstream f(name,std::ios::binary|std::ios::ate);if(!f)return false;
 const auto length=f.tellg();if(length<12||uint64_t(length)>limit+65536)return false;f.seekg(0);
 char id[4]{};uint32_t size=0;f.read(id,4);if(std::memcmp(id,"RIFF",4))return false;
 f.read(reinterpret_cast<char*>(&size),4);const uint64_t end=uint64_t(size)+8;
 f.read(id,4);if(!f||std::memcmp(id,"WAVE",4)||end>uint64_t(length)||end<12)return false;
 bool fmt=false,data=false;
 while(f&&uint64_t(f.tellg())+8<=end){
  f.read(id,4);f.read(reinterpret_cast<char*>(&size),4);if(!f)return false;
  const uint64_t offset=uint64_t(f.tellg()),next=offset+size+(size&1);if(next>end)return false;
  if(!std::memcmp(id,"fmt ",4)){
   if(fmt||size<16||size>40)return false;f.read(reinterpret_cast<char*>(&clip.format),std::min(size_t(size),sizeof(WAVEFORMATEX)));fmt=true;
  }else if(!std::memcmp(id,"data",4)){
   if(data||!size||size>limit)return false;clip.bytes.resize(size);f.read(clip.bytes.data(),size);data=true;
  }
  if(!f)return false;f.seekg(std::streamoff(next));
 }
 const auto& w=clip.format;
 return fmt&&data&&w.wFormatTag==WAVE_FORMAT_PCM&&(w.nChannels==1||w.nChannels==2)&&
  (w.wBitsPerSample==8||w.wBitsPerSample==16)&&w.nSamplesPerSec>=8000&&w.nSamplesPerSec<=96000&&
  w.nBlockAlign==w.nChannels*w.wBitsPerSample/8&&w.nAvgBytesPerSec==w.nSamplesPerSec*w.nBlockAlign&&
  clip.bytes.size()%w.nBlockAlign==0;
}
class Audio {
 struct Voice {IDirectSoundBuffer* buffer=nullptr;uint64_t serial=0;};
 IDirectSound8* ds=nullptr;IDirectSoundBuffer* bed=nullptr;IDirectSoundBuffer* prize=nullptr;bool prizeActive=false,prizePlaying=false,prizeResume=false;float prizeGain=0;
 IDirectSoundBuffer* failure=nullptr;bool failureActive=false,failurePlaying=false,failureResume=false;float failureGain=0;
 std::array<IDirectSoundBuffer*,7> clips{};std::array<Voice,16> voices{};
 uint64_t serial=0,previous=0;bool musicStarted=false,bedPlaying=false;float musicGain=0;
 void releaseVoices(){for(auto&v:voices)if(v.buffer){v.buffer->Stop();v.buffer->Release();v={};}}
 IDirectSoundBuffer* load(const std::wstring& name,size_t limit){
  PcmClip c;if(!readPcm(name,limit,c))return nullptr;
  DSBUFFERDESC desc{};desc.dwSize=sizeof(desc);desc.dwFlags=DSBCAPS_CTRLVOLUME|DSBCAPS_GLOBALFOCUS|DSBCAPS_LOCSOFTWARE;
  desc.dwBufferBytes=DWORD(c.bytes.size());desc.lpwfxFormat=&c.format;IDirectSoundBuffer* b=nullptr;
  if(FAILED(ds->CreateSoundBuffer(&desc,&b,nullptr)))return nullptr;
  void*p1=nullptr,*p2=nullptr;DWORD n1=0,n2=0;
  if(FAILED(b->Lock(0,desc.dwBufferBytes,&p1,&n1,&p2,&n2,0))){b->Release();return nullptr;}
  std::memcpy(p1,c.bytes.data(),n1);if(n2)std::memcpy(p2,c.bytes.data()+n1,n2);
  b->Unlock(p1,n1,p2,n2);b->SetVolume(DSBVOLUME_MIN);return b;
 }
 void playBed(){if(!bed)return;if(!musicStarted){bed->SetCurrentPosition(0);musicStarted=true;}bed->SetVolume(DSBVOLUME_MIN);bedPlaying=SUCCEEDED(bed->Play(0,0,DSBPLAY_LOOPING));musicGain=0;}
public:
 ~Audio(){close();}Audio()=default;Audio(const Audio&)=delete;Audio& operator=(const Audio&)=delete;
 bool enabled=true,running=false,focused=true;unsigned bedVolume=45,cueVolume=80,masterVolume=100;
 void setLevels(AudioLevels levels){
  levels.normalize();
  if(masterVolume==unsigned(levels.value[0])&&bedVolume==unsigned(levels.value[1])&&cueVolume==unsigned(levels.value[2]))return;
  masterVolume=unsigned(levels.value[0]);bedVolume=unsigned(levels.value[1]);cueVolume=unsigned(levels.value[2]);
  for(auto& voice:voices)if(voice.buffer)voice.buffer->SetVolume(volume(float(masterVolume*cueVolume)/100.f));
 }
 static LONG volume(float percent){percent=std::clamp(percent,0.f,100.f);return percent>0?LONG(std::max(-10000.,2000*std::log10(double(percent)/100))):DSBVOLUME_MIN;}
 void init(HWND window,const std::wstring& folder,bool studio=true){
  if(!enabled)return;if(!ds){if(FAILED(DirectSoundCreate8(nullptr,&ds,nullptr)))return;if(FAILED(ds->SetCooperativeLevel(window,DSSCL_NORMAL))){close();return;}}
  if(studio){if(!bed)bed=load(folder+L"music.wav",64*1024*1024);if(!prize)prize=load(folder+L"prize.wav",64*1024*1024);}
  const wchar_t* names[]={L"move.wav",L"accept.wav",L"back.wav",L"open.wav",L"spawn.wav",L"error.wav",L"slider.wav"};
  for(size_t i=0;i<clips.size();++i)if(!clips[i])clips[i]=load(folder+names[i],2*1024*1024);
  previous=GetTickCount64();
 }
 void initFailure(HWND window,const std::wstring& folder){init(window,folder,false);if(ds&&!failure)failure=load(folder+L"failure.wav",64*1024*1024);}
 void startFailure(){if(!enabled||!failure)return;running=false;prizeActive=false;if(bed)bed->Stop();bedPlaying=false;if(prize)prize->Stop();prizePlaying=false;releaseVoices();failureActive=true;failureResume=false;failureGain=0;failure->Stop();failure->SetCurrentPosition(0);failure->SetVolume(DSBVOLUME_MIN);failurePlaying=focused&&SUCCEEDED(failure->Play(0,0,DSBPLAY_LOOPING));}
 void endFailure(){failureActive=failurePlaying=failureResume=false;failureGain=0;if(failure)failure->Stop();}
 bool failureReady()const{return failure!=nullptr;}
 void start(){if(!enabled)return;endFailure();running=true;focused=true;playBed();}
  
 void stopBed(){running=false;prizeActive=false;}
 void startPrize(){running=false;if(bed)bed->Stop();bedPlaying=false;musicGain=0;releaseVoices();prizeActive=true;prizeGain=0;prizeResume=false;if(prize){prize->Stop();prize->SetCurrentPosition(0);prize->SetVolume(DSBVOLUME_MIN);prizePlaying=enabled&&focused&&SUCCEEDED(prize->Play(0,0,0));}}
 void endPrize(bool resume){prizeActive=false;prizeResume=false;if(prize)prize->Stop();prizePlaying=false;prizeGain=0;if(resume){running=true;if(focused&&enabled)playBed();}}
 bool prizeReady()const{return prize!=nullptr;}
 void update(){
  const uint64_t tick=GetTickCount64();const float dt=previous?std::min(float(tick-previous)/1000.f,.1f):0;previous=tick;
  const float target=running&&focused&&enabled?1.f:0.f;
  musicGain+=std::clamp(target-musicGain,-dt/.15f,dt/.15f);
  if(bed&&bedPlaying){bed->SetVolume(volume(float(masterVolume*bedVolume)/100.f*musicGain));if(musicGain<=0&&target==0){bed->Stop();bedPlaying=false;}}
  const float prizeTarget=prizeActive&&focused&&enabled?1.f:0.f;prizeGain+=std::clamp(prizeTarget-prizeGain,-dt/.12f,dt/.22f);if(prize&&prizePlaying){prize->SetVolume(volume(float(masterVolume*bedVolume)/100.f*prizeGain));DWORD flags=0;if(FAILED(prize->GetStatus(&flags))||!(flags&DSBSTATUS_PLAYING))prizePlaying=false;else if(prizeGain<=0&&prizeTarget==0){prize->Stop();prizePlaying=false;}}
  const float failTarget=failureActive&&focused&&enabled?1.f:0.f;failureGain+=std::clamp(failTarget-failureGain,-dt/.15f,dt/.08f);if(failure&&failurePlaying)failure->SetVolume(volume(float(masterVolume*bedVolume)/100.f*failureGain));
  for(auto&v:voices)if(v.buffer){DWORD flags=0;if(FAILED(v.buffer->GetStatus(&flags))||!(flags&DSBSTATUS_PLAYING)){v.buffer->Release();v={};}}
 }
 void focus(bool on){if(focused==on)return;focused=on;if(!on){if(bed)bed->Stop();bedPlaying=false;musicGain=0;prizeResume=prizePlaying;if(prize)prize->Stop();prizePlaying=false;prizeGain=0;failureResume=failurePlaying;if(failure)failure->Stop();failurePlaying=false;failureGain=0;releaseVoices();}else if(failureActive&&failureResume&&enabled&&failure){failurePlaying=SUCCEEDED(failure->Play(0,0,DSBPLAY_LOOPING));failureResume=false;}else if(prizeActive&&prizeResume&&enabled&&prize){prizePlaying=SUCCEEDED(prize->Play(0,0,0));prizeResume=false;}else if(running&&enabled)playBed();}
 bool musicReady()const{return bed!=nullptr;}
 bool effectsReady()const{return std::all_of(clips.begin(),clips.end(),[](auto*b){return b!=nullptr;});}
 bool ready()const{return musicReady()&&effectsReady();}
 size_t voiceCount()const{return size_t(std::count_if(voices.begin(),voices.end(),[](const Voice&v){return v.buffer!=nullptr;}));}
 bool voicesMuted()const{for(const auto&v:voices)if(v.buffer){LONG level=0;if(FAILED(v.buffer->GetVolume(&level))||level!=DSBVOLUME_MIN)return false;}return true;}
 bool cue(Cue cue){
  if(!enabled||!focused||!ds||cue==Cue::None)return false;update();
  size_t index=1;switch(cue){case Cue::Move:index=0;break;case Cue::Slider:index=6;break;case Cue::Back:index=2;break;case Cue::Open:index=3;break;case Cue::Spawn:index=4;break;case Cue::Error:index=5;break;default:break;}
  if(!clips[index])return false;IDirectSoundBuffer* b=nullptr;if(FAILED(ds->DuplicateSoundBuffer(clips[index],&b)))return false;
   
   
  const LONG level=volume(float(masterVolume*cueVolume)/100.f);b->SetVolume(level==DSBVOLUME_MIN?DSBVOLUME_MIN+1:DSBVOLUME_MIN);b->SetVolume(level);
  b->SetCurrentPosition(0);if(FAILED(b->Play(0,0,0))){b->Release();return false;}
  auto it=std::find_if(voices.begin(),voices.end(),[](const Voice&v){return v.buffer==nullptr;});
  if(it==voices.end()){it=std::min_element(voices.begin(),voices.end(),[](const Voice&a,const Voice&b){return a.serial<b.serial;});it->buffer->Stop();it->buffer->Release();}
  *it={b,++serial};return true;
 }
 void stop(){endFailure();prizeActive=false;prizeResume=false;prizePlaying=false;prizeGain=0;if(prize)prize->Stop();running=false;if(bed)bed->Stop();bedPlaying=false;musicGain=0;releaseVoices();}
 void close(){stop();if(failure)failure->Release();failure=nullptr;if(bed)bed->Release();bed=nullptr;if(prize)prize->Release();prize=nullptr;for(auto*&b:clips){if(b)b->Release();b=nullptr;}if(ds)ds->Release();ds=nullptr;musicStarted=false;previous=0;}
};
}
