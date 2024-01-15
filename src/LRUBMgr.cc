#include "LRUBMgr.h"

namespace DB {

LRUBMgr::~LRUBMgr() {
    lruList.clear();
    frame2iter.clear();
}

void LRUBMgr::accessFrame(int frameID, int isWrite) {
    for (auto iter = lruList.begin(); iter != lruList.end(); iter++) {
        if (iter->frameID == frameID) {
            lruList.erase(iter);
            break;
        }
    }
    lruList.push_front(LRUElem{
        frameID,
    });
}

void LRUBMgr::RemoveLRUEle(int frameID) {
    lruList.pop_back();
}

int LRUBMgr::getVictimFrameID() {
    return lruList.back().frameID;
}

}