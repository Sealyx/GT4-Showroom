#include "../source/NativePreview.hpp"
#include <cassert>
#include <cstdio>
using namespace gt4;
int main(){
 assert(!nativePreview::onEntityRender(nullptr));
 alignas(4) std::array<uint8_t,0x1C> material{};game::field<int16_t>(material.data(),0x18)=2;game::field<uint32_t>(material.data(),4)=0xFFAABBCC;
 for(size_t i=0;i<12;++i)material[0xC+i]=uint8_t(i);void* mats[]={material.data(),material.data()};
 alignas(4) std::array<uint8_t,0x60> geometry{};game::field<int>(geometry.data(),0x24)=2;game::field<void**>(geometry.data(),0x20)=mats;
 alignas(4) std::array<uint8_t,0x24> atomic{};game::field<void*>(atomic.data(),0x18)=geometry.data();nativePreview::Pins pins;nativePreview::pinAtomic(atomic.data(),&pins);assert(pins.valid&&pins.values.size()==1&&game::field<int16_t>(material.data(),0x18)==3);
 game::field<uint32_t>(material.data(),4)=0;material[0xC]=99;nativePreview::restore(pins.values);assert(game::field<uint32_t>(material.data(),4)==0xFFAABBCC&&material[0xC]==0&&game::field<int16_t>(material.data(),0x18)==3);
 --game::field<int16_t>(material.data(),0x18);game::field<int16_t>(material.data(),0x18)=0;nativePreview::Pins invalid;nativePreview::pinAtomic(atomic.data(),&invalid);assert(!invalid.valid&&invalid.values.empty());
 // Only the isolated private target is suppressed. Ordinary world entities
 // still take the original native render path, including Quantum Break's hook.
 alignas(4) std::array<uint8_t,0x38> preview{},world{};nativePreview::target=preview.data();assert(nativePreview::onEntityRender(preview.data()));assert(!nativePreview::onEntityRender(world.data()));nativePreview::target=nullptr;nativePreview::result.reset();
 for(int id:{400,554,20042})assert(game::previewSeed(id)==game::previewSeed(id)&&game::previewSeed(id)>=0);
 std::puts("Native preview: material dedup/refcount and volatile-state restoration, invalid references, target-only draw suppression and deterministic model seeds passed. No GTA engine was run.");
}
