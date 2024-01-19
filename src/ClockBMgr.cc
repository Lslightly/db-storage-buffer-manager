#include "ClockBMgr.h"
#include "defer/defer.hpp"
#include "spdlog/spdlog.h"
#include <cstdlib>

namespace DB {

void ClockBMgr::accessFrame(int frameID, int isWrite) {
    eval->startMaintain();
    defer [&]{eval->endMaintain();};
    if (!full && !isFrameExist(frameID)) {
        if (isCycleEnd()) { // 说明已经将clock填充满，之后就可以开始替换了
            full = true;
        }
        setCurrentFrameID(frameID);
        incCurrent();
    } else if (full) {
        if (isFrameExist(frameID)) {
            setFrameReferenced(frameID);
        } else { // new frame after replacing victim frame
            setCurrentFrameID(frameID);
            incCurrent();
        }
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
    frame2idx.erase(frameID); // frameID不再在clock中
}

}