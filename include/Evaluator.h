#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "spdlog/fmt/bundled/core.h"
#include <cstddef>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <list>

namespace DB {

class Eval {
public:
    using TimeUnit = std::chrono::microseconds;
    using TimerTy = std::chrono::system_clock::time_point;
    using Clock = std::chrono::high_resolution_clock;
    Eval(std::string outfile="eval.txt"): outfName(outfile), readCnt(0), writeCnt(0), missCnt(0), hitCnt(0), dirtyVictimCnt(0), cleanVictimCnt(0), readTs(), writeTs() {}
    void readOnce() { readCnt++; }
    void writeOnce() { writeCnt++; }
    void miss() { missCnt++; }
    void hit() { hitCnt++; }
    void dirtyVictim() { dirtyVictimCnt++; }
    void cleanVictim() { cleanVictimCnt++; }
    void startTimer() { timer = Clock::now(); }
    void endTimer(bool isWrite) {
        auto duration = std::chrono::duration_cast<TimeUnit>
        (
            Clock::now()
            - timer
        );
        if (!isWrite)
            readTs.push_back(duration);
        else
            writeTs.push_back(duration);
    }
    void endWriteDirtyTimes() {
        finishWriteDirtys = std::chrono::duration_cast<TimeUnit>
        (
            Clock::now()
            - timer
        );
    }
    void startMaintain() {
        getVictimMaintainTimer = Clock::now();
    }
    void endMaintain() {
        maintainTimes.push_back(
            std::chrono::duration_cast<TimeUnit>(Clock::now() - getVictimMaintainTimer)
        );
    }
    void describe() {
        auto IO = readCnt+writeCnt;
        auto bufAccess = missCnt+hitCnt;
        auto victims = dirtyVictimCnt+cleanVictimCnt;
        TimeUnit totalT = finishWriteDirtys;
        TimeUnit readTotalT = TimeUnit::zero();
        for (auto readT: readTs) {
            readTotalT += readT;
        }
        totalT += readTotalT;
        TimeUnit writeTotalT = TimeUnit::zero();
        for (auto writeT: writeTs) {
            writeTotalT += writeT;
        }
        totalT += writeTotalT;
        TimeUnit maintainT = TimeUnit::zero();
        for (auto mT: maintainTimes) {
            maintainT += mT;
        }

#define CntAndPercent(a, b, total) \
    a, b, total,\
    percent(a, total), percent(b, total), 100.0
#define CntAndPercent4(a, b, c, total) \
    a, b, c, total,\
    percent(a, total), percent(b, total), percent(c, total), 100.0

        auto* outf = std::fopen(outfName.c_str(), "w");
        fmt::print(outf, "statistic:\n\
read, write, IO,\n\
{}, {}, {}\n\
{:03.2f}%, {:03.2f}%, {:03.2f}%\n\
\n\
miss, hit, buf access\n\
{}, {}, {}\n\
{:03.2f}%, {:03.2f}%, {:03.2f}%\n\
\n\
dirty victims, clean victims, victims\n\
{}, {}, {}\n\
{:03.2f}%, {:03.2f}%, {:03.2f}%\n\
\n\
read T(us), write T(us), end write dirties T(us), total T(us)\n\
{}, {}, {}, {}\n\
{:03.2f}%, {:03.2f}%, {:03.2f}%, {:03.2f}%\n\
\n\
maintain replacement data struct(us), \n\
{}\n\
{:03.2f}%",
CntAndPercent(readCnt, writeCnt, IO),
CntAndPercent(missCnt, hitCnt, bufAccess),
CntAndPercent(dirtyVictimCnt, cleanVictimCnt, victims),
CntAndPercent4(readTotalT.count(), writeTotalT.count(), finishWriteDirtys.count(), totalT.count()),
maintainT.count(),
percent(maintainT.count(), totalT.count())
);
        std::fclose(outf);
    }
    void setOutFile(std::string ofileName) { outfName = ofileName; }
    void clear() {
        readCnt = writeCnt = missCnt = hitCnt = dirtyVictimCnt = cleanVictimCnt = 0;
        readTs.clear();
        writeTs.clear();
        finishWriteDirtys = TimeUnit::zero();
    }
private:
    template <typename T>
    double percent(T small, T big) { return static_cast<double>(small)*1.0/static_cast<double>(big)*100.0; }
    std::string outfName;
    int readCnt, writeCnt;  // 读写次数
    int missCnt, hitCnt;    // 失效次数，命中次数
    int dirtyVictimCnt, cleanVictimCnt;
    std::list<TimeUnit> readTs, writeTs;
    TimeUnit finishWriteDirtys;
    TimerTy timer;
    TimerTy getVictimMaintainTimer; // 记录如LRU为了维持数据结构花费的时间
    std::list<TimeUnit> maintainTimes;
};

}

#endif