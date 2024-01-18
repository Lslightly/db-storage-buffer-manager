#include "BMgr.h"
#include "DSMgr.h"
#include <list>
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
protected:
    virtual void accessFrame(int frameID, int isWrite) override;
private:
    std::unordered_map<int, LRUIter> frame2iter;
    /*
        MRU(front, cnt smallest), ..., LRU(end, cnt largest)
    */
    LRUList lruList;
};
}