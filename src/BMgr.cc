#include "DSMgr.h"
#include "BMgr.h"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace DB {

void initFrame(bFrame &frame, int pageID) {
    auto str = std::to_string(pageID);
    strncpy(frame.field, str.c_str(), str.length()+1);
}

bFrame GlobalBuf[BUFSIZE];
bFrame* getFrameFromBuf(int frameID) { return &GlobalBuf[frameID]; }
void setFrameToBuf(int frameID, bFrame &frame) {
    spdlog::debug("setting frame {}'s new frame {}", frameID, frame.field);
    GlobalBuf[frameID] = frame;
}

BMgr::BMgr(std::string dbname): dbFile(dbname)
{
    for (int i = 0; i < BUFSIZE; i++) {
        freeFrames.insert(i);
    }
    dsMgr = std::make_shared<DSMgr>();
    dsMgr->OpenFile(dbFile);
    for (int i = 0; i < BUFSIZE; i++) {
        ftop[i] = FrameUnused;
        ptof[i] = nullptr;
    }
}

BMgr::~BMgr() {
    WriteDirtys();
    for (int i = 0; i < BUFSIZE; i++) {
        auto* bcb = ptof[i];
        if (bcb) {
            auto* bcbNext = bcb->next;
            delete bcb;
            while (bcbNext) {
                bcb = bcbNext;
                bcbNext = bcb->next;
                delete bcb;
            }
        }
    }
    dsMgr->CloseFile();
}

int BMgr::FixPage(int pageID, int _prot)
{
    auto* bcb = p2bcb(pageID);
    if (bcb) {
        spdlog::debug("pageID {} in buffer, return frameID {}", bcb->frameID);
        return bcb->frameID;
    }

    spdlog::debug("pageID {} not in buffer, need to load from database file", pageID);
    auto [freeFrameID, found] = findFreeFrameForPage(pageID);
    bFrame newFrame;
    if (!found) {
        freeFrameID = SelectVictim();
        newFrame = dsMgr->ReadPage(pageID);
    }

    addNewBCB(freeFrameID, pageID);
    setFrameToPage(freeFrameID, pageID);
    setFrameToBuf(freeFrameID, newFrame);
    return freeFrameID;
}

NewPage BMgr::FixNewPage() {
    spdlog::debug("Fix New Page...");

    auto newPageId = dsMgr->GetNumPages();
    spdlog::debug("new page id {}", newPageId);
    dsMgr->IncNumPages();

    dsMgr->SetUse(newPageId, PageStatus::Used);

    bFrame newFrame;
    initFrame(newFrame, newPageId);
    
    auto size = dsMgr->WritePage(newPageId, newFrame);
    spdlog::debug("write {} with size {}", newPageId, size);

    auto [newFrameID, found] = findFreeFrameForPage(newPageId);
    if (!found) { // use victimFrameId as new frame
        spdlog::debug("no free frame, finding victim...");
        newFrameID = SelectVictim();
    } else {
        spdlog::debug("find free frame {0}", newFrameID);
    }

    addNewBCB(newFrameID, newPageId);
    setFrameToBuf(newFrameID, newFrame);
    setFrameToPage(newFrameID, newPageId);
    return {newFrameID, newPageId};
}

int BMgr::UnfixPage(int pageID) {
    auto* bcb = p2bcb(pageID);
    if (!bcb) {
        spdlog::error("page id {} has no bcb!", pageID);
        exit(1);
    }

    if (bcb->latch != 0) {
        bcb->latch--;
    }
    return bcb->frameID;
}

int BMgr::NumFreeFrames() {
    return BUFSIZE-freeFrames.size();
}

int BMgr::SelectVictim() {
    int victimFrameID = 0;

    auto* bcb = f2bcb(victimFrameID);
    if (!bcb) {
        spdlog::error("victim frame {} should have bcb.", victimFrameID);
        exit(1);
    }

    auto pageID = bcb->pageID;
    spdlog::debug("victim (frame, page) = ({}, {})", victimFrameID, pageID);

    RemoveBCB(bcb, pageID);

    if (bcb->dirty) {
        spdlog::debug("writing dirty victim page {}", pageID);
        dsMgr->WritePage(pageID, *getFrameFromBuf(victimFrameID));
        bcb->dirty = 0;
    }

    delete bcb;

    return victimFrameID;
}

int BMgr::Hash(int pageID) {
    return pageID % BUFSIZE;
}

void BMgr::RemoveBCB(BCB* ptr, int pageID) {
    auto val = Hash(pageID);
    auto* bcb = ptof[pageID];
    if (ptr == bcb) { // ptr is the head of ptof[pageID]
        ptof[pageID] = ptr->next; // remove ptr from ptof[pageID]
        return;
    }

    auto* bcbPrev = bcb;
    bcb = bcb->next;
    while (bcb) {
        if (bcb == ptr) {
            bcbPrev->next = bcb->next; // remove ptr and relink bcbPrev and bcb->next
            return;
        }
        bcbPrev = bcb;
        bcb = bcb->next;
    }
    spdlog::error("should find bcb of pageID {} in bcb list.", pageID);
    exit(1);
}

void BMgr::RemoveLRUEle(int frameID) {
    // TODO
}

void BMgr::SetDirty(int frameID) {
    auto* bcb = f2bcb(frameID);
    bcb->dirty = 1;
}

void BMgr::UnsetDirty(int frameID) {
    auto* bcb = f2bcb(frameID);
    bcb->dirty = 0;   
}

void BMgr::WriteDirtys() {
    for (int frameID = 0; frameID < BUFSIZE; frameID++) {
        if (isFrameUsed(frameID)) {
            auto* bcb = f2bcb(frameID);
            auto pageID = f2p(frameID);
            if (bcb->dirty == 1) {
                dsMgr->WritePage(pageID, *getFrameFromBuf(frameID));
                bcb->dirty = 0;
            }
        }
    }
}

void BMgr::PrintFrame(int frameID) {
    spdlog::info("frameID: {} with frame content (pageID: {})", frameID, getFrameFromBuf(frameID)->field);
}

std::tuple<int, bool> BMgr::findFreeFrameForPage(int pageID) {
    spdlog::debug("findFreeFrameForPage...");
    if (isBufferFull()) {
        return std::make_tuple(0, false);
    }
    return std::make_tuple(*freeFrames.begin(), true);
}


}