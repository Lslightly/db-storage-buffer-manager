#include "Mgr.h"
#include <functional>
#include <memory>
#include <system_error>
#include <utility>
#include "spdlog/spdlog.h"

namespace DB {

bFrame GlobalBuf[BUFSIZE];

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

int DSMgr::WritePage(int frame_id, bFrame frm)
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

BMgr::BMgr()
{
    dsmgr = std::make_shared<DSMgr>(new DSMgr());
    dsmgr->OpenFile(dbFile);
    for (int i = 0; i < BUFSIZE; i++) {
        ftop[i] = 0;
        ptof[i] = nullptr;
    }
}

BMgr::~BMgr() {
    dsmgr->CloseFile();
}

int BMgr::FixPage(int pageID, int prot)
{
    auto* bcb = p2bcb(pageID, Hash);
    if (bcb) {
        return bcb->frameID;
    } else {
        // TODO
        return 0;
    }
}

NewPage BMgr::FixNewPage() {
    dsmgr->IncNumPages();
    auto newpageid = dsmgr->GetNumPages();
    dsmgr->SetUse(newpageid, UseStatus::Used);

    auto newframeid = Hash(newpageid);
    auto* newbcb = new BCB(newpageid, newframeid);

    // link to ptof[newpageid] bucket list
    if (ptof[newpageid] == nullptr) {
        ptof[newpageid] = newbcb;
    } else {
        auto* listp = ptof[newpageid];
        while (listp->next != nullptr) {
            listp = listp->next;
        }
        listp->next = newbcb;
    }
    
    ftop[newframeid] = newpageid;
    return {newpageid, newframeid};
}

int BMgr::UnfixPage(int pageID) {
    auto* bcb = p2bcb(pageID, Hash);
    if (bcb) {
        // TODO
        return bcb->frameID;
    } else {
        // TODO
        return 0;
    }
}

int BMgr::NumFreeFrames() {
    // TODO
    return 0;
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

BCB* BMgr::p2bcb(int pageID, int(*hash)(int)) {
    int val = hash(pageID);
    auto bcb = ptof[val];
    while (bcb) {
        if (bcb->pageID == pageID) {
            return bcb;
        }
        bcb = bcb->next;
    }
    return nullptr;
}

int BMgr::f2p(int frameID) {
    return ftop[frameID];
}

} // namespace DB