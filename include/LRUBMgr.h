#include "BMgr.h"
#include "DSMgr.h"
#include <list>
#include <string>
#include <unordered_map>

namespace DB {
class LRUBMgr: public BMgr {
public:
    LRUBMgr(DSMgr* initmgr, Eval* ev): BMgr(initmgr, ev, "LRUBMgr") {}
    virtual ~LRUBMgr();

    struct LRUElem {
        int frameID;
    };
    using LRUList = std::list<LRUElem>;
    using LRUIter = LRUList::iterator;
    /*
        This function removes the LRU element from the list.
    */
    virtual void RemoveLRUEle(int frid) override;
    virtual int getVictimFrameID() override;
    virtual void accessFrame(int frameID, int isWrite) override;
private:
    std::unordered_map<int, LRUIter> frame2iter;
    /*
        MRU(front, cnt smallest), ..., LRU(end, cnt largest)
    */
    LRUList lruList;
};


class LRU_K_BMgr: public BMgr {
public:
    LRU_K_BMgr(DSMgr* initmgr, Eval* ev, int k): BMgr(initmgr, ev, "LRU-"+std::to_string(k)), K(k) {}
    virtual ~LRU_K_BMgr();
    struct LRUElem {
        int frameID;
        int cnt;
    };
    using LRUList = std::list<LRUElem>;
    using LRUIter = LRUList::iterator;
    struct IterInfo {
        LRUIter iter;
        bool isInGEList; // 是否在 >= K列表里面
    };
    virtual void RemoveLRUEle(int frameID) override;
    virtual int getVictimFrameID() override;
    virtual void accessFrame(int frameID, int isWrite) override;
private:
    bool full;
    int K = 2;
    using Frame2IterInfo = std::unordered_map<int, IterInfo>;
    LRUList ltKlist, geKList;
    Frame2IterInfo frame2iterinfo;
};

}