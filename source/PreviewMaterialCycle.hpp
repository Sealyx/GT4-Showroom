#pragma once
namespace gt4 {
// The setup/reset pair is an indivisible capture transaction. The native
// material restore list is global, so no nested vehicle draw is allowed here.
template<class Vehicle,class Setup,class Draw,class Reset>
void previewMaterialCycle(Vehicle vehicle,Setup setup,Draw draw,Reset reset){
 setup(vehicle);struct Restore{Vehicle vehicle;Reset& callback;~Restore(){callback(vehicle);}} restore{vehicle,reset};draw(vehicle);
}
}
