#include "Mgr.h"
#include <functional>
#include <memory>
#include <system_error>
#include <tuple>
#include <utility>
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace DB {

bFrame GlobalBuf[BUFSIZE];
bFrame* getFrameFromBuf(int frameID) { return &GlobalBuf[frameID]; }
void setFrameToBuf(int frameID, bFrame &frame) {
    GlobalBuf[frameID] = frame;
}

int DSMgr::OpenFile(std::string filename) {
    try {
        currFile.open(filename, std::ios::in | std::ios::out);
    } catch (const std::system_error& e) {
        return e.code().value();
    }
    return 0;
}

int DSMgr::CloseFile() {
    try {
        currFile.close();
    } catch (const std::system_error& e) {
        return e.code().value();
    }
    return 0;
}

bFrame DSMgr::ReadPage(int page_id)
{
    bFrame tmp;
    currFile.seekg(page_id*FRAMESIZE);
    currFile.read(tmp.field, FRAMESIZE);
    return tmp;
}

int DSMgr::WritePage(int frame_id, bFrame& frm)
{
    currFile.seekp(frame_id*FRAMESIZE);
    auto before = currFile.tellp();
    currFile.write(frm.field, FRAMESIZE);
    auto after = currFile.tellp();
    return after - before;
}

int DSMgr::Seek(int offset, int pos)
{
    if (pos == 0) { // 从文件开始
        currFile.seekg(offset, std::ios::beg);
        currFile.seekp(offset, std::ios::beg);
    } else if (pos == 1) { // 从当前位置
        currFile.seekg(offset, std::ios::cur);
        currFile.seekp(offset, std::ios::cur);
    } else if (pos == 2) { // 从文件结束
        currFile.seekg(offset, std::ios::end);
        currFile.seekp(offset, std::ios::end);
    } else {
        return -1; // 错误的pos值
    }
    return 0;
}

std::fstream &DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
    numPages++;
}

int DSMgr::GetNumPages()
{
    return numPages;
}

void DSMgr::SetUse(int page_id, int use_bit)
{
    pages[page_id] = use_bit;
}

int DSMgr::GetUse(int page_id)
{
    return pages[page_id];
}

BMgr::BMgr(std::string dbname): dbFile(dbname)
{
    numUsedFrames = 0;
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
    dsMgr->WritePage(newPageId, newFrame);

    auto [newFrameId, found] = findFreeFrameForPage(newPageId);
    if (!found) { // use victimFrameId as new frame
        spdlog::debug("no free frame, finding victim...");
        auto victimFrameId = SelectVictim();
        auto victimPageId = f2p(victimFrameId);
        spdlog::debug("victim (frame, page) = ({}, {})", victimFrameId, victimPageId);

        auto* bcb = p2bcb(victimPageId);
        if (bcb->dirty) {
            spdlog::debug("write dirty page...");
            dsMgr->WritePage(victimPageId, *getFrameFromBuf(victimFrameId));
        }
        bcb->pageID = newPageId;
        bcb->count = 1;
        bcb->dirty = 0;
        bcb->latch = 0;
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
    return BUFSIZE-numUsedFrames;
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
    int pageHash = Hash(pageID);
    auto tryFrameID = pageHash;
    while (isFrameUsed(tryFrameID)) {
        tryFrameID = incFrame(tryFrameID);
    }
    return std::make_tuple(tryFrameID, true);
}

} // namespace DB