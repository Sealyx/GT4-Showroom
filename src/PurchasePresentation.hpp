#pragma once

namespace gt4 {
enum class PurchaseStep { Closed, Inspect, Invoice, Handoff, Offer, Processing, Complete };

struct PurchasePresentation {
    PurchaseStep step = PurchaseStep::Closed;
    int choice = 0;
    PurchaseStep previous = PurchaseStep::Closed;
    double began = 0, opened = 0, blackBegan = 0, leaving = -1;

    bool studioView() const { return step==PurchaseStep::Inspect || step==PurchaseStep::Invoice || step==PurchaseStep::Handoff; }
    bool blackView() const { return active() && !studioView(); }
    bool active() const { return step != PurchaseStep::Closed; }

    void show(PurchaseStep next, double now) {
        if(next==PurchaseStep::Offer) blackBegan=now;
        previous = step;
        step = next;
        began = now;
        choice = 0;
    }

    void begin(double now) { opened = now; show(PurchaseStep::Inspect, now); }

    static float slideProgress(double age) {
        const float t=std::clamp(float(age/.40),0.f,1.f);
        return t*t*(3.f-2.f*t);
    }
    float entryOffset(double now,float width) const {
        return step>=PurchaseStep::Offer ? -.16f*width*(1-slideProgress(now-began)) : 0.f;
    }
    int buttons() const {
        if (step == PurchaseStep::Handoff || step == PurchaseStep::Processing || step == PurchaseStep::Closed) return 0;
        return step == PurchaseStep::Invoice || step == PurchaseStep::Offer ? 2 : 1;
    }

    UIRect button(int index, float width, float height) const {
        if (step == PurchaseStep::Inspect) return {width*.44f,height*.737f,width*.12f,height*.155f};
        if (step == PurchaseStep::Invoice) return {width*(index ? .521f : .333f),height*.824f,width*.168f,height*.051f};
        return {width*(step==PurchaseStep::Complete ? .673f : index ? .778f : .589f),height*.49f,width*.166f,height*.053f};
    }

    void tick(double now) {
        if (step == PurchaseStep::Handoff && now - began >= 1.25) show(PurchaseStep::Offer,now);
        if (step == PurchaseStep::Processing && now - began >= 2.45) show(PurchaseStep::Complete,now);
    }
};
}
