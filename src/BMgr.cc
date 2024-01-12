#include "Mgr.h"
#include <functional>
#include <memory>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace DB {

bFrame GlobalBuf[BUFSIZE];
bFrame* getFrameFromBuf(int frameID) { return &GlobalBuf[frameID]; }
void setFrameToBuf(int frameID, bFrame &frame) {
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

int BMgr::FixPage(int pageID, int prot)
{
    auto* bcb = p2bcb(pageID);
    if (bcb) {
        return bcb->frameID;
    } else {
        // TODO
        return 0;
    }
}

NewPage BMgr::FixNewPage() {
    spdlog::debug("Fix New Page...");

    auto newPageId = dsMgr->GetNumPages();
    spdlog::debug("new page id {}", newPageId);
    dsMgr->IncNumPages();

    dsMgr->SetUse(newPageId, PageStatus::Used);
    bFrame newFrame;
    auto size = dsMgr->WritePage(newPageId, newFrame);
    spdlog::debug("write {} with size {}", newPageId, size);

    auto [newFrameId, found] = findFreeFrameForPage(newPageId);
    if (!found) { // use victimFrameId as new frame
        spdlog::debug("no free frame, finding victim...");
        auto victimFrameId = SelectVictim();
        auto victimPageId = f2p(victimFrameId);
        spdlog::debug("victim (frame, page) = ({}, {})", victimFrameId, victimPageId);

        auto* bcb = replacePageIDInBCB(victimPageId, newPageId);
        
        spdlog::debug("writing new frame...");
        setFrameToBuf(victimFrameId, newFrame);

        spdlog::debug("map frame({}) -> page({})", victimFrameId, newPageId);
        setFrameToPage(victimFrameId, newPageId);
        return {victimFrameId, newPageId};
    }

    setFrameToBuf(newFrameId, newFrame);

    spdlog::debug("find free frame {0}, map frame({0}) -> page({1})", newFrameId, newPageId);
    setFrameToPage(newFrameId, newPageId);
    addNewBCB(newFrameId, newPageId);
    return {newFrameId, newPageId};
}

int BMgr::UnfixPage(int pageID) {
    auto* bcb = p2bcb(pageID);
    if (bcb) {
        // TODO
        return bcb->frameID;
    } else {
        // TODO
        return 0;
    }
}

int BMgr::NumFreeFrames() {
    return BUFSIZE-freeFrames.size();
}

int BMgr::SelectVictim() {
    // TODO
    return 0;
}

int BMgr::Hash(int pageID) {
    return pageID % BUFSIZE;
}

void BMgr::RemoveBCB(BCB* ptr, int pageID) {
    // TODO
}

void BMgr::RemoveLRUEle(int frid) {
    // TODO
}

void BMgr::SetDirty(int frameID) {
    // TODO
}

void BMgr::UnsetDirty(int frameID) {
    // TODO
}

void BMgr::WriteDirtys() {
    // TODO
}

void BMgr::PrintFrame(int frameID) {
    // TODO
}

std::tuple<int, bool> BMgr::findFreeFrameForPage(int pageID) {
    spdlog::debug("findFreeFrameForPage...");
    if (isBufferFull()) {
        return std::make_tuple(0, false);
    }
    return std::make_tuple(*freeFrames.begin(), true);
}

BCB* BMgr::replacePageIDInBCB(int victimPageID, int newPageID) {
    auto victimHash = Hash(victimPageID);
    auto* bcb = ptof[victimHash];
    if (!bcb) {
        spdlog::error("ensure bcb exists when calling replacePageID(ptof[{}]).", victimHash);
        exit(1);
    }

    BCB* res = nullptr;
    if (bcb->pageID == victimPageID) { // the head of the list is the wanted bcb
        res = bcb;
        ptof[victimHash] = res->next; // remove res from list
    } else {
        auto* bcbPrev = bcb;
        bcb = bcb->next;
        while (bcb) {
            if (bcb->pageID == victimPageID) {
                res = bcb;
                bcbPrev->next = res->next; // remove res from list
                break;
            }
            bcbPrev = bcb;
            bcb = bcb->next;
        }
        if (!bcb) {
            spdlog::error("ensure bcb exists when calling replacePageID({} does not exist in bcb list of ptof[{}]).", victimPageID, victimHash);
            exit(1);
        }
    }

    // set res as the head of ptof[newHash]
    auto newHash = Hash(newPageID);
    auto* oldHead = ptof[newHash];
    ptof[newHash] = res;
    res->next = oldHead;

    // write victim page
    if (res->dirty) {
        spdlog::debug("writing victim page {}", victimPageID);
        dsMgr->WritePage(victimPageID, *getFrameFromBuf(res->frameID));
    }

    res->pageID = newPageID;
    res->count = 1;
    res->dirty = 0;
    res->latch = 0;

    return res;
}

}