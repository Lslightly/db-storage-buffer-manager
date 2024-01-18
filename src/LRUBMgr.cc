#include "LRUBMgr.h"
#include "defer/defer.hpp"

namespace DB {

LRUBMgr::~LRUBMgr() {
    lruList.clear();
    frame2iter.clear();
}

void LRUBMgr::accessFrame(int frameID, int isWrite) {
    eval->startMaintain();
    defer [&]{ eval->endMaintain(); };

    if (frame2iter.find(frameID) != frame2iter.end()) {
        auto iter = frame2iter[frameID];
        lruList.erase(iter);
    }
    lruList.push_front(LRUElem{
        frameID,
    });
    frame2iter[frameID] = lruList.begin();

}

void LRUBMgr::RemoveLRUEle(int frameID) {
    eval->startMaintain();
    defer [&]{ eval->endMaintain(); };

    lruList.erase(frame2iter.at(frameID));
    frame2iter.erase(frameID);
}

int LRUBMgr::getVictimFrameID() {
    return lruList.back().frameID;
}

/*
LRU
---
LRU-K
*/

LRU_K_BMgr::~LRU_K_BMgr() {
    ltKlist.clear();
    geKList.clear();
    frame2iterinfo.clear();
}

void LRU_K_BMgr::accessFrame(int frameID, int isWrite) {
    eval->startMaintain();
    defer [&]{ eval->endMaintain(); };

    if (frame2iterinfo.find(frameID) != frame2iterinfo.end()) {
        auto info = frame2iterinfo[frameID];
        if (info.isInGEList) { // 在 >= K list中只需要替换到最前端
            auto elem = *info.iter;
            geKList.erase(info.iter);
            geKList.push_front(elem);
            frame2iterinfo[frameID] = IterInfo{
                geKList.begin(),
                true,
            };
        } else {
            ++info.iter->cnt;
            if (info.iter->cnt >= K) { // 晋升到 >= K list中
                auto elem = *info.iter;
                ltKlist.erase(info.iter);
                geKList.push_front(elem);
                frame2iterinfo[frameID] = IterInfo{
                    geKList.begin(),
                    true,
                };
            }
        }
        return ;
    }

    // 没有在任何列表中出现，应该插入 < K list中   
    auto elem = LRUElem{
        frameID,
        1,
    };
    ltKlist.push_front(elem);
    frame2iterinfo[frameID] = IterInfo{
        ltKlist.begin(),
        false,
    };

    return ;
}

int LRU_K_BMgr::getVictimFrameID() {
    eval->startMaintain();
    defer [&]{ eval->endMaintain(); };

    if (!ltKlist.empty()) {
        return ltKlist.back().frameID;
    }
    return geKList.back().frameID;
}

void LRU_K_BMgr::RemoveLRUEle(int frameID) {
    eval->startMaintain();
    defer [&]{ eval->endMaintain(); };

    auto info = frame2iterinfo.at(frameID);
    if (info.isInGEList) {
        geKList.erase(info.iter);
    } else {
        ltKlist.erase(info.iter);
    }

    frame2iterinfo.erase(frameID);
    return ;
}

}