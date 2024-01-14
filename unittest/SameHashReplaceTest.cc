#include "gtest/gtest.h"
#include <gtest/gtest_prod.h>
#include <string>
#include "BMgr.h"
#include "DSMgr.h"
#include "Evaluator.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

namespace DB {

class SameHashReplaceTest: public testing::Test {
public:
    void SetUp() override {
        eval = new Eval;
        dsmgr = new DSMgr(eval, false);
        dsmgr->OpenFile("data.dbf");
        mgr = new BMgr(dsmgr, eval);
    }
    void TearDown() override {
        delete mgr;
    }
    DB::Eval* eval;
    DB::BMgr* mgr;
    DB::DSMgr* dsmgr;
};

TEST_F(SameHashReplaceTest, SameHash) {
    spdlog::set_level(spdlog::level::debug);
    int page1 = 1;
    int page2 = page1+DB::BUFSIZE;
    auto page1hash = mgr->Hash(page1), page2hash = mgr->Hash(page2);
    ASSERT_EQ(page1hash, page2hash);
    auto frame1 = mgr->FixPage(page1, 0);
    auto* bcb1 = mgr->p2bcb(page1);
    ASSERT_EQ(bcb1->count, 1);
    ASSERT_EQ(
        std::to_string(page1),
        std::string(getFrameFromBuf(frame1)->field)
    );
    spdlog::debug("frame content {}", getFrameFromBuf(frame1)->field);

    auto frame2 = mgr->FixPage(page2, 0);
    auto* bcb2 = mgr->p2bcb(page2);
    ASSERT_EQ(bcb2->count, 1);
    std::string tag2(getFrameFromBuf(frame2)->field);
    ASSERT_EQ(
        std::to_string(page2),
        std::string(getFrameFromBuf(frame2)->field)
    );
    ASSERT_NE(frame1, frame2);
}

}