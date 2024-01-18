#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <argparse/argparse.hpp>
#include <defer/defer.hpp>
#include <list>
#include <map>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>
#include <vector>
#include "ClockBMgr.h"
#include "DSMgr.h"
#include "BMgr.h"
#include "LRUBMgr.h"
#include "Evaluator.h"
#include "spdlog/common.h"

/*
In our project, you are required to perform a trace-driven experiment to demonstrate your
implemental result. The trace has been generated according to the Zipf distribution. There
are total 500,000 page references in the trace, which are restricted to a set of pages whose
numbers range from 1 to 50,000. Each trace record has the format as “x, ###”, where x is
0(read) or 1(write) and ### is the referenced page number. You are required to scan the
trace file and print out the total I/Os between memory and disk. The buffer is supposed
empty at the beginning of your experiment. All the 50,000 pages involved in the trace
need to be first materialized in the disk, which corresponds to the directory-based file
data.dbf
*/

const std::string OptDBName = "-db-name";
const std::string OptLogLevel = "-log-level";
const std::string OptBenchFile = "-bench-file";


void runBench(std::string benchFile, DB::BMgr* mgr, DB::Eval* eval);

void createDB(std::string dbname) {
    DB::Eval eval;
    DB::DSMgr dsmgr(&eval, true);
    dsmgr.OpenFile(dbname);
    DB::BMgr mgr(&dsmgr, &eval);

    for (int i = 0; i < DB::MAXPAGES; i++) {
        mgr.FixNewPage();
    }
    spdlog::info("finish creating data.dbf...");
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("DBDriver", "0.0.1");
    program.add_argument(OptDBName)
            .help("database file name")
            .default_value(std::string{"data.dbf"});
    program.add_argument(OptLogLevel)
            .help("log level. 6 for off, 1 for debug, 2 for info, 4 for err")
            .default_value(2)
            .scan<'i', int>();
    program.add_argument(OptBenchFile)
            .help("benchmark file")
            .default_value(std::string{"test/data-5w-50w-zipf.txt"});
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        std::exit(1);
    }

    spdlog::set_level(spdlog::level::level_enum(program.get<int>(OptLogLevel)));
    spdlog::set_pattern("[%^%l%$] %v");

    if (program.is_used(OptBenchFile)) {
        std::list<std::tuple<DB::Eval*, DB::DSMgr*, DB::BMgr*>> mgrList;
        
        // naive
        auto* tmpEval = new DB::Eval;
        auto* tmpDSMgr = new DB::DSMgr(tmpEval, false);
        DB::BMgr* tmpBMgr = new DB::BMgr(tmpDSMgr, tmpEval);
        mgrList.push_back(std::make_tuple(tmpEval, tmpDSMgr, tmpBMgr));

        // LRU
        tmpEval = new DB::Eval;
        tmpDSMgr = new DB::DSMgr(tmpEval, false);
        tmpBMgr = new DB::LRUBMgr(tmpDSMgr, tmpEval);
        mgrList.push_back(std::make_tuple(tmpEval, tmpDSMgr, tmpBMgr));

        // LRU-k
        for (auto k = 2; k < 5; k++) {
            tmpEval = new DB::Eval;
            tmpDSMgr = new DB::DSMgr(tmpEval, false);
            tmpBMgr = new DB::LRU_K_BMgr(tmpDSMgr, tmpEval, k);
            mgrList.push_back(std::make_tuple(tmpEval, tmpDSMgr, tmpBMgr));
        }

        // Clock
        tmpEval = new DB::Eval;
        tmpDSMgr = new DB::DSMgr(tmpEval, false);
        tmpBMgr = new DB::ClockBMgr(tmpDSMgr, tmpEval);
        mgrList.push_back(std::make_tuple(tmpEval, tmpDSMgr, tmpBMgr));
        
        auto dbname = program.get(OptDBName);
        std::string workloadFileName = program.get<std::string>(OptBenchFile);

        for (auto [eval, dsmgr, mgr]: mgrList) {
            spdlog::info("creating data.dbf...");
            spdlog::set_level(spdlog::level::off);
            createDB(dbname);
            spdlog::set_level(spdlog::level::level_enum(program.get<int>(OptLogLevel)));
            eval->setOutFile(mgr->getName()+".txt");
            dsmgr->OpenFile(dbname);
            runBench(workloadFileName, mgr, eval);
            delete mgr;
            delete dsmgr;
            delete eval;
        }
    }
    return 0;
}

void runBench(std::string benchFileName, DB::BMgr* mgr, DB::Eval* eval) {
    spdlog::info("Running benchmark for {} with replace method {}...", benchFileName, mgr->getName());

    std::fstream benchFile;
    try {
        benchFile.open(benchFileName, std::ios::in);
    } catch (const std::system_error& e) {
        spdlog::error("error occurs when opening file {} with error code {}", benchFileName, e.code().value());
        exit(e.code().value());
    }
    defer [&]{benchFile.close();};

    std::vector<DB::Op> oplist;
    oplist.reserve(50000);

    std::string line;
    while (std::getline(benchFile, line)) {
        auto iter = line.find_first_of(",");
        auto op = std::atoi(line.substr(0, iter).c_str());
        auto pageID = std::atoi(line.substr(iter + 1).c_str());
        oplist.push_back(DB::Op{
            bool(op),
            pageID,
        });
    }

    mgr->ExecOpList(oplist);
    spdlog::debug("finish exec op list");
    eval->describe();
}
