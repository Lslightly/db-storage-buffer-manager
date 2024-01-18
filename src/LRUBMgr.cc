#include "LRUBMgr.h"

namespace DB {

LRUBMgr::~LRUBMgr() {
    lruList.clear();
    frame2iter.clear();
}

void LRUBMgr::accessFrame(int frameID, int isWrite) {
    eval->startMaintain();

    if (frame2iter.find(frameID) != frame2iter.end()) {
        auto iter = frame2iter[frameID];
        lruList.erase(iter);
    }
    lruList.push_front(LRUElem{
        frameID,
    });
    frame2iter[frameID] = lruList.begin();

    eval->endMaintain();
}

void LRUBMgr::RemoveLRUEle(int frameID) {
    eval->startMaintain();

    lruList.erase(frame2iter[frameID]);
    frame2iter.erase(frameID);

    eval->endMaintain();
}

int LRUBMgr::getVictimFrameID() {
    return lruList.back().frameID;
}

}