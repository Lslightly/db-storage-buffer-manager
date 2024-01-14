#ifndef DSBUF_MGR_H
#define DSBUF_MGR_H

#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <tuple>


namespace DB {

const int FRAMESIZE = 4096;
const int MAXPAGES = 50000;

struct bFrame {
    char field[FRAMESIZE];
};

enum PageStatus {
    Used,
    Free,
};

class DSMgr {
public:
    DSMgr(bool trunc=false): truncFile(trunc), numPages(0) {}
    /*
        This function is called anytime a file needs to be opened for reading or writing. The prototype for this function is OpenFile(String filename) and returns an error code. The function opens the file specified by the filename.
    */
    int OpenFile(std::string filename);
    /*
        This function is called when the data file needs to be closed. The protoype is CloseFile() and returns an error code. This function closes the file that is in current use. This function should only be called as the database is changed or a the program closes.
    */
    int CloseFile();
    /*
        This function is called by the FixPage function in the buffer manager. This prototype is ReadPage(page_id, bytes) and returns what it has read in. This function calls fseek() and fread() to gain data from a file.
    */
    bFrame ReadPage(int page_id);
    /*
        This function is called whenever a page is taken out of the buffer. The prototype is WritePage(page_id, frm) and returns how many bytes were written. This function calls fseek() and fwrite() to save data into a file.
    */
    int WritePage(int page_id, bFrame& frm);
    //  This function moves the file pointer.
    int Seek(int offset, int pos);
    //  This function returns the current file.
    std::fstream& GetFile();
    //  This function increments the page counter.
    void IncNumPages();
    //  This function returns the page counter.
    int GetNumPages();
    /*
        This function sets the bit in the pages array. This array keeps track of the pages that are being used. If all records in a page are deleted, then that page is not really used anymore and can be reused again in the database. In order to know if a page is reusable, the array is checked for any use_bits that are set to zero. The fixNewPage function firsts checks this array for a use_bit of zero. If one is found, the page is reused. If not, a new page is allocated.
    */
    void SetUse(int page_id, int use_bit);
    //  This function returns the current use_bit for the corresponding page_id.
    int GetUse(int page_id);
private:
    std::string fileName;
    std::fstream currFile;
    int numPages;
    int pages[MAXPAGES];
    bool truncFile;
};

struct NewPage {
    int frameID;
    int pageID;
};

} // namespace DB

#endif