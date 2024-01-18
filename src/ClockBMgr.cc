#include "ClockBMgr.h"
#include "defer/defer.hpp"
#include "spdlog/spdlog.h"
#include <cstdlib>

namespace DB {

void ClockBMgr::accessFrame(int frameID, int isWrite) {
    eval->startMaintain();
    defer [&]{eval->endMaintain();};
    if (!full) {
        if (isCycleEnd()) { // 说明已经将clock填充满，之后就可以开始替换了
            full = true;
        }
        incCurrent();
        setCurrentFrameID(frameID);
    } else {
        setCurrentFrameID(frameID); // Victim frame has been replaced. Set new frameID and referenced
        incCurrent();
    }
}

int ClockBMgr::getVictimFrameID() {
    eval->startMaintain();
    defer [&]{eval->endMaintain();};

    if (!full) {
        spdlog::error("get victim when buffer is not full?");
        exit(1);
    }
    while (isReferenced()) {
        unsetReferenced();
        incCurrent();
    }
    return getFrameOfCurrent();
}

void ClockBMgr::RemoveLRUEle(int frameID) {
    // Do nothing. No need to change current. It will be accessed soon.
}

}