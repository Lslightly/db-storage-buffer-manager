#include <cstdlib>
#include <exception>
#include <iostream>
#include <argparse/argparse.hpp>
#include <defer/defer.hpp>
#include <spdlog/spdlog.h>
#include "Mgr.h"
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
            .default_value(6)
            .scan<'i', int>();
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        std::exit(1);
    }
    DB::BMgr mgr(program.get<std::string>(OptDBName));
    spdlog::set_level(spdlog::level::level_enum(program.get<int>(OptLogLevel)));
    if (program[OptCreateDB] == true) {
        spdlog::info("creating data.dbf...");
        for (int i = 0; i < DB::MAXPAGES; i++) {
            mgr.FixNewPage();
        }
    }
    return 0;
}