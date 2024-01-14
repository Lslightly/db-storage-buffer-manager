#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <argparse/argparse.hpp>
#include <defer/defer.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include "DSMgr.h"
#include "BMgr.h"
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

const std::string OptCreateDB = "-create-db";
const std::string OptDBName = "-db-name";
const std::string OptLogLevel = "-log-level";
const std::string OptBenchFile = "-bench-file";

void runBench(std::string benchFile, DB::BMgr& mgr);

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("DBDriver", "0.0.1");
    program.add_argument(OptCreateDB)
            .help("create data.dbf with 50000 pages")
            .default_value(false)
            .implicit_value(true);
    program.add_argument(OptDBName)
            .help("database file name")
            .default_value(std::string("data.dbf"));
    program.add_argument(OptLogLevel)
            .help("log level. 6 for off, 1 for debug, 2 for info, 4 for err")
            .default_value(2)
            .scan<'i', int>();
    program.add_argument(OptBenchFile)
            .help("benchmark file")
            .default_value(std::string("test/data-5w-50w-zipf.txt"));
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        std::exit(1);
    }

    DB::DSMgr dsmgr(program.get<bool>(OptCreateDB));
    dsmgr.OpenFile(program.get<std::string>(OptDBName));
    DB::BMgr mgr(&dsmgr);

    spdlog::set_level(spdlog::level::level_enum(program.get<int>(OptLogLevel)));
    spdlog::set_pattern("[%^%l%$] %v");

    if (program.is_used(OptCreateDB) && program.is_used(OptBenchFile)) {
        spdlog::error("{} and {} are exclusive.", OptCreateDB, OptBenchFile);
        exit(1);
    }
    if (program[OptCreateDB] == true) {
        spdlog::info("creating data.dbf...");
        for (int i = 0; i < DB::MAXPAGES; i++) {
            mgr.FixNewPage();
        }
        return 0;
    }
    if (program[OptBenchFile] == true) {
        std::string workloadFileName = program.get<std::string>(OptBenchFile);
        runBench(workloadFileName, mgr);
    }
    return 0;
}

void runBench(std::string benchFileName, DB::BMgr& mgr) {
    spdlog::info("Running benchmark for {}...", benchFileName);

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

    mgr.ExecOpList(oplist);
}
