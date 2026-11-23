#pragma once
namespace gt4 {
inline float ease(float value);
struct SelectionPanelMotion {
    double began=0;
    float from=0;
    double enterDuration=.20;
    bool visible=false;
    float alpha(double now) const {
        const float t=ease(float((now-began)/(visible?enterDuration:.20)));
        return from+((visible?1.f:0.f)-from)*t;
    }
    void show(bool next,double now) {
        if(next==visible) return;
        from=alpha(now);began=now;visible=next;
    }
    float offset(double now,float scale) const { return -14.f*scale*(1-alpha(now)); }
};
}
