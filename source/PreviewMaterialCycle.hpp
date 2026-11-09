#pragma once

namespace gt4
{
    template <class Vehicle, class Setup, class Draw, class Reset>
    void previewMaterialCycle(Vehicle vehicle, Setup setup, Draw draw, Reset reset)
    {
        setup(vehicle);

        struct Restore
        {
            Vehicle vehicle;
            Reset& callback;

            ~Restore()
            {
                callback(vehicle);
            }
        } restore{vehicle, reset};

        draw(vehicle);
    }

    // keep both texture passes inside the usual setup/reset pair, with no initialization repeated between them.
    template <class Vehicle, class Setup, class Draw, class Reset, class Clean>
    void previewMaterialPasses(Vehicle vehicle, Setup setup, Draw draw, Reset reset, Clean clean)
    {
        previewMaterialCycle(vehicle, setup, draw, reset);
        if (clean(vehicle)) {
            previewMaterialCycle(vehicle, setup, draw, reset);
        }
    }

} // namespace gt4
