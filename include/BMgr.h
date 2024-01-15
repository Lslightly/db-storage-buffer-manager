#ifndef BMGR_H
#define BMGR_H

#include "Evaluator.h"
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "DSMgr.h"

namespace DB {

const int BUFSIZE = 1024;

void initFrame(bFrame& frame, int pageID);

extern bFrame GlobalBuf[BUFSIZE];
bFrame* getFrameFromBuf(int frameID);
void setFrameToBuf(int frameID, bFrame& frame);

class BCB {
public:
    BCB(int pageid, int frameid)
    : pageID(pageid), frameID(frameid), latch(0), count(0), dirty(0), next(nullptr) {}
    int pageID;
    int frameID;
    int latch;
    int count;
    int dirty;
    BCB* next;
};

struct Op {
    bool isWrite;
    int pageID;
};


class BMgr {
public:
    BMgr(DSMgr* initmgr, Eval *ev, std::string name="naiveBMgr");
    virtual ~BMgr();
    // Interface functions
    /*
        return the frame_id of the page
        the `pageID` is in the record_id of the record
        the function looks whether the page is in the buffer already and returns the frame_id. If the page is not in the buffer yet, it selects a victim page, if needed, and loads in the requested page.
    */
    int FixPage(int page_id, int isWrite);
    /*
        This returns a page_id and a frame_id.
        This function is used when a new page is needed on an insert, index split, or object creation. The page_id is returned in order to assign to the record_id and metadata. This function will find an empty page that the File and Access Manager can use to store some data.
    */
    NewPage FixNewPage();
    /*
        This returns a frame_id. This function is the compliment to a FixPage or FixNewPage call. This function decrements the fix count on the frame. If the count reduces to zero, then the latch on the page is removed and the frame can be removed if selected. The page_id is translated to a frame_id and it may be unlatched so that it can be chosen as a victim page if the count on the page has been reduced to zero.
    */
    int UnfixPage(int page_id);
    /*
        This function looks at the buffer and returns the number of buffer pages that are free and able to be used. This is especially useful for the N-way sort for the query processor. It returns an integer from 0 to BUFFERSIZE-1(1023).
    */
    int NumFreeFrames();

    // Internal Functions

    // Get victim frame id. Do nothing to BCB and dirty pages.
    virtual int getVictimFrameID();
    /*
        This function selects a frame to replace. If the dirty bit of the selected frame is set then the page needs to be written on to the disk. The according BCB will also be removed. 
    */
    virtual int SelectVictim();
    /*
        It takes the page_id as the parameter and returns the frame id.
    */
    static int Hash(int page_id);
    /*
        This function removes the Buffer Control Block for the page_id from the array. This is only called if the SelectVictim() function needs to replace a frame.
    */
    void RemoveBCB(BCB * ptr, int page_id);
    /*
        This function removes the LRU element from the list.
    */
    virtual void RemoveLRUEle(int frid);
    /*
        This function sets the dirty bit for the frame_id. This dirty bit is used to know whether or not to write out the frame. A frame must be written if the contents have been modified in any way. This includes any directory pages and data pages. If the bit is 1, it will be written. If this bit is zero, it will not be written.
    */
    void SetDirty(int frame_id);
    /*
        This function assigns the dirty_bit for the corresponding frame_id to zero. The main reason to call this function is when the setDirty() function has been called but the page is actually part of a temporary relation. In this case, the page will not actually need to be written, because it will not want to be saved.
    */
    void UnsetDirty(int frame_id);
    /*
        This function must be called when the system is shut down. The purpose of the function is to write out any pages that are still in the buffer that may need to be written. It will only write pages out to the file if the dirty_bit is one.
    */
    void WriteDirtys();
    /*
        This function prints out the contents of the frame described by the frame_id. 
    */
    void PrintFrame(int frame_id);
    /*
        This function receive a series of read/write operations.
    */
    void ExecOpList(std::vector<Op> &oplist);
    // find BCB for pageID, return nullptr if not found.
    BCB* p2bcb(int pageID) {
        int val = Hash(pageID);
        auto* bcb = ptof[val];
        spdlog::debug("find bcb in ptof[{}] for page {}", val, pageID);
        while (bcb) {
            if (bcb->pageID == pageID) {
                return bcb;
            }
            bcb = bcb->next;
        }
        return nullptr;
    }

    // frame to bcb
    BCB* f2bcb(int frameID) {
        auto pageID = f2p(frameID);
        auto* bcb = p2bcb(pageID);
        if (!bcb) {
            spdlog::error("should find bcb for (frame: {}, page: {})", frameID, pageID);
            exit(1);
        }
        return bcb;
    }

    std::string getName() { return name; }
protected:
    std::set<int> freeFrames;
    const int FrameUnused = -1;
    int ftop[BUFSIZE]; // -1 means frame is not used
    BCB* ptof[BUFSIZE];
    DSMgr* dsMgr;
    Eval* eval;

    /**
     * @brief consider FixPage as an access to frameID
     * 
     * @param frameID 
     * @param isWrite 
     */
    virtual void accessFrame(int frameID, int isWrite);

    /**
     * @brief find free frame for pageID
     * 
     * @param pageID 
     * @return std::tuple<int, bool> [frameID, found]
     */
    std::tuple<int, bool> findFreeFrameForPage(int pageID);

    // (frameID+1) % BUFSIZE
    int incFrame(int frameID) { return (frameID+1) % BUFSIZE; }
    /**
     * @brief add new BCB for (frameID, pageID) at the begin of ptof[pageID] bcb list
     * 
     * @param frameID 
     * @param pageID 
     * @return BCB* 
     */
    BCB* addNewBCB(int frameID, int pageID) {
        auto pageHash = Hash(pageID);
        auto* bcbListP = ptof[pageHash];
        auto* newBCB = new BCB(pageID, frameID);
        if (!bcbListP) { // no BCB list yet
            spdlog::debug("add new BCB(frame: {}, page: {}) in nil list in ptof[{}]", frameID, pageID, pageHash);
            ptof[pageHash] = newBCB;
            return newBCB;
        }

        newBCB->next = bcbListP;
        ptof[pageHash] = newBCB;
        spdlog::debug("add new BCB(frameID: {}, pageID: {}) at the begin of ptof[{}] list", frameID, pageID, pageHash);

        return newBCB;
    }

    int f2p(int frameID) {
        return ftop[frameID];
    }
    bool isBufferFull() { return freeFrames.size() == 0; }
    bool isFrameUsed(int frameID) { return ftop[frameID] != FrameUnused; }
    void setFrameToPage(int frameID, int pageID) {
        spdlog::debug("set frame {} to page {}", frameID, pageID);
        if (!isFrameUsed(frameID)) {
            freeFrames.erase(frameID);
        }
        ftop[frameID] = pageID;
    }
    void unsetFrame(int frameID) {
        if (isFrameUsed(frameID)) {
            freeFrames.insert(frameID);
        }
        ftop[frameID] = FrameUnused;
    }
    std::string name;
};

}

#endif