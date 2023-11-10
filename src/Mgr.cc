#include "Mgr.h"
#include <functional>
#include <system_error>

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
    return bFrame();
}

int DSMgr::WritePage(int frame_id, bFrame frm)
{
    return 0;
}

int DSMgr::Seek(int offset, int pos)
{
    return 0;
}

std::fstream &DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
}

int DSMgr::GetNumPages()
{
    return 0;
}

void DSMgr::SetUse(int page_id, int use_bit)
{
}

int DSMgr::GetUse(int page_id)
{
    return 0;
}

BMgr::BMgr()
{
}

int BMgr::FixPage(int pageID, int prot)
{
    auto* bcb = p2bcb(pageID, hash);
    if (bcb) {
        return bcb->frameID;
    } else {
        // TODO
        return 0;
    }
}

NewPage BMgr::FixNewPage() {
    // TODO
    return NewPage();
}

int BMgr::UnfixPage(int pageID) {
    auto* bcb = p2bcb(pageID, hash);
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

BCB* BMgr::p2bcb(int pageID, const std::function<int(int)>& hash) {
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