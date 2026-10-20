#include "../source/Audio.hpp"
#include <cassert>
#include <cstdio>
using namespace gt4;
int main(int argc,char**argv){
 assert(argc==2);const std::filesystem::path folder=argv[1];PcmClip clip;
 assert(readPcm(folder/L"music.wav",64*1024*1024,clip));assert(clip.bytes.size()>20*1024*1024);
 assert(!readPcm(folder/L"music.wav",20*1024*1024,clip));
 const auto bad=std::filesystem::temp_directory_path()/("GT4AudioMalformed-"+std::to_string(GetCurrentProcessId())+".wav");
 {std::ofstream f(bad,std::ios::binary);f.write("RIFF\xFF\xFF\xFF\x7F" "WAVE",12);}assert(!readPcm(bad,1024,clip));
 {std::ofstream f(bad,std::ios::binary);uint32_t riff=38,fmt=16,bytes=2;WAVEFORMATEX w{};w.wFormatTag=1;w.nChannels=2;w.nSamplesPerSec=44100;w.wBitsPerSample=16;w.nBlockAlign=0;w.nAvgBytesPerSec=176400;f.write("RIFF",4);f.write(reinterpret_cast<char*>(&riff),4);f.write("WAVEfmt ",8);f.write(reinterpret_cast<char*>(&fmt),4);f.write(reinterpret_cast<char*>(&w),16);f.write("data",4);f.write(reinterpret_cast<char*>(&bytes),4);f.write("\0\0",2);}assert(!readPcm(bad,1024,clip));std::filesystem::remove(bad);
 WNDCLASSW c{};c.lpfnWndProc=DefWindowProcW;c.hInstance=GetModuleHandleW(nullptr);c.lpszClassName=L"GT4AudioTest";RegisterClassW(&c);
 HWND window=CreateWindowW(c.lpszClassName,L"Silent audio checks",WS_OVERLAPPEDWINDOW,0,0,32,32,nullptr,nullptr,c.hInstance,nullptr);assert(window);
 Audio audio;audio.bedVolume=audio.cueVolume=0;audio.init(window,folder.wstring()+L"/");assert(audio.ready());
 assert(Audio::volume(0)==DSBVOLUME_MIN&&Audio::volume(100)==DSBVOLUME_MAX&&Audio::volume(150)==DSBVOLUME_MAX);assert(Audio::volume(50)<0);
 audio.enabled=false;audio.start();assert(!audio.running&&!audio.cue(Cue::Open));audio.enabled=true;audio.start();assert(audio.running);
 for(Cue cue:{Cue::Open,Cue::Move,Cue::Move,Cue::Accept,Cue::Back,Cue::Spawn,Cue::Error})assert(audio.cue(cue));
 assert(audio.voiceCount()>=6&&audio.voicesMuted());
 for(int i=0;i<40;++i)assert(audio.cue(Cue::Move));assert(audio.voiceCount()==16&&audio.voicesMuted());
 audio.stopBed();assert(!audio.running&&audio.voiceCount()>0);audio.focus(false);assert(audio.voiceCount()==0);audio.focus(true);
 assert(audio.prizeReady());audio.startPrize();assert(!audio.running);audio.focus(false);audio.focus(true);audio.update();audio.endPrize(true);assert(audio.running);audio.startPrize();audio.stopBed();audio.update();audio.endPrize(false);assert(!audio.running);
 audio.stop();audio.close();assert(!audio.ready()&&!audio.musicReady()&&audio.voiceCount()==0);
 audio.init(window,(folder/L"missing").wstring()+L"/");assert(!audio.ready());audio.close();DestroyWindow(window);
 std::puts("Full music, standalone prize cue + six separate SFX loaded; prize switching/resume/cleanup; bounded overlapping voices, duplicate-buffer mute, focus, tails, cleanup and malformed/oversized WAV rejection passed (silent checks).");
}
