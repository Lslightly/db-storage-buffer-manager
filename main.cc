#include <cstdlib>
#include <exception>
#include <iostream>
#include <argparse/argparse.hpp>
#include <defer/defer.hpp>
#include <spdlog/spdlog.h>
#include "Mgr.h"

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

const std::string OptCreateDB = "create-db";

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("BufMgr", "0.0.1");
    program.add_argument(OptCreateDB)
            .help("create data.dbf with 50000 pages")
            .default_value(false);
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        spdlog::error(program);
        std::exit(1);
    }
    DB::BMgr mgr;
    if (program[OptCreateDB] == true) {
        spdlog::info("creating data.dbf...");
        for (int i = 0; i < DB::MAXPAGES; i++) {
            mgr.FixNewPage();
        }
    }
    return 0;
}