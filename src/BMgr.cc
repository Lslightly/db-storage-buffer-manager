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
#include "Evaluator.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace DB {

void initFrame(bFrame &frame, int pageID) {
    auto str = std::to_string(pageID);
    strncpy(frame.field, str.c_str(), str.length()+1);
    spdlog::debug("init page {} with content {}", pageID, frame.field);
}

bFrame GlobalBuf[BUFSIZE];
bFrame* getFrameFromBuf(int frameID) { return &GlobalBuf[frameID]; }
void setFrameToBuf(int frameID, bFrame &frame) {
    spdlog::debug("store frame {} with content {} in buf", frameID, frame.field);
    GlobalBuf[frameID] = frame;
}

BMgr::BMgr(DSMgr* initDSMgr, Eval* ev, std::string n)
{
    name = n;
    for (int i = 0; i < BUFSIZE; i++) {
        freeFrames.insert(i);
    }
    dsMgr = initDSMgr;
    eval = ev;
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

int BMgr::FixPage(int pageID, int isWrite)
{
    auto* bcb = p2bcb(pageID);
    if (bcb) {
        eval->hit();
        spdlog::debug("pageID {} in buffer, return frameID {}", bcb->frameID);
        bcb->count++;
        if (isWrite) {
            spdlog::debug("make page dirty {}", pageID);
            bcb->dirty = 1;
        } else {
            spdlog::debug("read page {}", pageID);
        }
        accessFrame(bcb->frameID, isWrite);
        return bcb->frameID;
    }

    eval->miss();
    spdlog::debug("pageID {} not in buffer, need to load from database file", pageID);
    bFrame newFrame = dsMgr->ReadPage(pageID);

    auto [freeFrameID, found] = findFreeFrameForPage(pageID);
    if (!found) {
        freeFrameID = SelectVictim();
    }

    bcb = addNewBCB(freeFrameID, pageID);
    bcb->count++;
    if (isWrite)
        bcb->dirty = 1;
    accessFrame(freeFrameID, isWrite);
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
    dsMgr->ReadPage(newPageId);
    spdlog::debug("write {} with size {}", newPageId, size);

    auto [newFrameID, found] = findFreeFrameForPage(newPageId);
    if (!found) { // use victimFrameId as new frame
        spdlog::debug("no free frame, finding victim...");
        newFrameID = SelectVictim();
    } else {
        spdlog::debug("find free frame {0}", newFrameID);
    }

    auto* bcb = addNewBCB(newFrameID, newPageId);
    bcb->count++;
    accessFrame(newFrameID, 1);
    setFrameToBuf(newFrameID, newFrame);
    setFrameToPage(newFrameID, newPageId);
    spdlog::debug("finish FixNewPage...");
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

int BMgr::getVictimFrameID() {
    return 0;
}

int BMgr::SelectVictim() {
    int victimFrameID = getVictimFrameID();
    RemoveLRUEle(victimFrameID);

    auto* bcb = f2bcb(victimFrameID);
    if (!bcb) {
        spdlog::error("victim frame {} should have bcb.", victimFrameID);
        exit(1);
    }

    auto pageID = bcb->pageID;
    spdlog::debug("victim (frame, page) = ({}, {})", victimFrameID, pageID);

    RemoveBCB(bcb, pageID);

    if (bcb->dirty) {
        eval->dirtyVictim();
        spdlog::debug("writing dirty victim page {}", pageID);
        dsMgr->WritePage(pageID, *getFrameFromBuf(victimFrameID));
        bcb->dirty = 0;
    } else {
        eval->cleanVictim();
    }

    delete bcb;

    spdlog::debug("finish dealing with victim (frame: {}, page: {})", victimFrameID, pageID);

    return victimFrameID;
}

int BMgr::Hash(int pageID) {
    return pageID % BUFSIZE;
}

void BMgr::RemoveBCB(BCB* ptr, int pageID) {
    auto pageHash = Hash(pageID);
    auto* bcb = ptof[pageHash];
    if (ptr == bcb) { // ptr is the head of ptof[pageID]
        ptof[pageHash] = ptr->next; // remove ptr from ptof[pageID]
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
    // do nothing
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
    spdlog::debug("writing dirties...");
    for (int frameID = 0; frameID < BUFSIZE; frameID++) {
        if (isFrameUsed(frameID)) {
            auto* bcb = f2bcb(frameID);
            auto pageID = f2p(frameID);
            if (bcb->dirty) {
                spdlog::debug("page {} dirty", pageID);
                dsMgr->WritePage(pageID, *getFrameFromBuf(frameID));
                bcb->dirty = 0;
            } else {
                spdlog::info("page {} is not dirty", f2p(frameID));
            }
        }
    }
}

void BMgr::PrintFrame(int frameID) {
    spdlog::info("frameID: {} with frame content (pageID: {})", frameID, getFrameFromBuf(frameID)->field);
}

void BMgr::ExecOpList(std::vector<Op> &oplist) {
    for (auto op: oplist) {
        auto pageID = op.pageID;
        eval->startTimer();
        auto frameID = FixPage(pageID, op.isWrite);
        if (op.isWrite) {
            // simulate write operation
            bFrame tmp;
            initFrame(tmp, pageID);
            setFrameToBuf(frameID, tmp);
        } else {
            // similuate read operation
            PrintFrame(frameID);
        }
        eval->endTimer(op.isWrite);
    }
    eval->startTimer();
    WriteDirtys();
    eval->endWriteDirtyTimes();
}

// do nothing
void BMgr::accessFrame(int frameID, int isWrite) {}

std::tuple<int, bool> BMgr::findFreeFrameForPage(int pageID) {
    spdlog::debug("findFreeFrameForPage...");
    if (isBufferFull()) {
        return std::make_tuple(0, false);
    }
    return std::make_tuple(*freeFrames.begin(), true);
}


}