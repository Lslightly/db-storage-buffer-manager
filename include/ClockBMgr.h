#ifndef CLOCK_BMGR_H
#define CLOCK_BMGR_H

#include "BMgr.h"
#include <vector>

namespace DB {

class ClockBMgr: public BMgr {
public:
    ClockBMgr(DSMgr* initmgr, Eval* ev): BMgr(initmgr, ev, "Clock") {
        current = 0;
        elemClock.resize(BUFSIZE);
        full = false;
    }
    ~ClockBMgr() { elemClock.clear(); }

    struct ClockElem {
        int frameID;
        bool referenced;
    };

    virtual void RemoveLRUEle(int frameID) override;
    virtual int getVictimFrameID() override;
    virtual void accessFrame(int frameID, int isWrite) override;
private:
    std::vector<ClockElem> elemClock;
    int current; // 0 ~ BUFSIZE-1
    void incCurrent() { current = (current+1)%BUFSIZE; }
    bool isCycleEnd() { return current == BUFSIZE-1; }
    void setCurrentFrameID(int frameID) {
        elemClock[current] = ClockElem{
            frameID,
            true,
        };
    }
    void unsetReferenced() { elemClock[current].referenced = false; }
    bool isReferenced() { return elemClock[current].referenced; }
    int getFrameOfCurrent() { return elemClock[current].frameID; }
    bool full;
};

}

#endif
