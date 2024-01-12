#include "Mgr.h"
#include <functional>
#include <memory>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>
#include "spdlog/logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

namespace DB {

int DSMgr::OpenFile(std::string filename) {
    try {
        fileName = filename;
        currFile.open(filename, std::ios::in | std::ios::out | std::ios::app);
    } catch (const std::system_error& e) {
        spdlog::error("error occurs when opening file {} with error code {}", filename, e.code().value());
        exit(e.code().value());
    }
    return 0;
}

int DSMgr::CloseFile() {
    try {
        currFile.close();
    } catch (const std::system_error& e) {
        spdlog::error("error occurs when closing file {} with error code {}", fileName, e.code().value());
        exit(e.code().value());
    }
    return 0;
}

bFrame DSMgr::ReadPage(int pageID)
{
    bFrame tmp;
    currFile.seekg(pageID*FRAMESIZE);
    currFile.read(tmp.field, FRAMESIZE);
    return tmp;
}

int DSMgr::WritePage(int pageID, bFrame& frm)
{
    currFile.seekp(pageID*FRAMESIZE);
    auto before = currFile.tellp();
    currFile.write(frm.field, FRAMESIZE);
    auto after = currFile.tellp();
    return after - before;
}

int DSMgr::Seek(int offset, int pos)
{
    if (pos == 0) { // 从文件开始
        currFile.seekg(offset, std::ios::beg);
        currFile.seekp(offset, std::ios::beg);
    } else if (pos == 1) { // 从当前位置
        currFile.seekg(offset, std::ios::cur);
        currFile.seekp(offset, std::ios::cur);
    } else if (pos == 2) { // 从文件结束
        currFile.seekg(offset, std::ios::end);
        currFile.seekp(offset, std::ios::end);
    } else {
        return -1; // 错误的pos值
    }
    return 0;
}

std::fstream &DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
    numPages++;
}

int DSMgr::GetNumPages()
{
    return numPages;
}

void DSMgr::SetUse(int page_id, int use_bit)
{
    pages[page_id] = use_bit;
}

int DSMgr::GetUse(int page_id)
{
    return pages[page_id];
}

} // namespace DB