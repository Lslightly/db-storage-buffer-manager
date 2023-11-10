#ifndef BUF_MGR_H
#define BUF_MGR_H

#include <string>
namespace DB {

const int BUFSIZE = 1024;
const int FRAMESIZE = 4096;
const int MAXPAGES = 50000;

struct bFrame {
    char field[FRAMESIZE];
};

extern bFrame GlobalBuf[BUFSIZE];

struct BCB {
    BCB();
    int pageID;
    int frameID;
    int latch;
    int count;
    int dirty;
    BCB* next;
};

// page/frame map
class PFMap {
public:
    PFMap() = delete;
    static BCB* p2bcb(int pageID);
    static int f2p(int frameID);
private:
    static BCB hashPage2BCB[BUFSIZE];
    static int tableFrame2Page[BUFSIZE];
};

class DSMgr {
public:
    DSMgr();
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
        This function is called whenever a page is taken out of the buffer. The prototype is WritePage(frame_id, frm) and returns how many bytes were written. This function calls fseek() and fwrite() to save data into a file.
    */
    int WritePage(int frame_id, bFrame frm);
    //  This function moves the file pointer.
    int Seek(int offset, int pos);
    //  This function returns the current file.
    FILE * GetFile();
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
    const std::string dbFile = "data.dbf";
private:
    FILE *currFile;
    int numPages;
    int pages[MAXPAGES];
};

using NewPage = struct{
    int pageID;
    int frameID;
};

class BMgr {
public:
    BMgr();
    // Interface functions
    /*
        return the frame_id of the page
        the `pageID` is in the record_id of the record
        the function looks whether the page is in the buffer already and returns the frame_id. If the page is not in the buffer yet, it selects a victim page, if needed, and loads in the requested page.
    */
    int FixPage(int page_id, int prot);
    /*
        This returns a page_id and a frame_id.
        This function is used when a new page is needed on an insert, index split, or object creation. The page_id is returned in order to assign to the record_id and metadata. This function will find an empty page that the File and Access Manager can use to store some data.
    */
    NewPage FixNewPage();
    /*
        This returns a frame_id. This function is the compliment to a FixPage or FixNewPage call. This function decrements the fix count on the frame. If the count reduces to zero, then the latch on the page is removed and the frame can be removed if selected. The page_id is translated to a frame_id and it may be unlatched so that it can be chosen as a victim page if the count on the page has been reduced to zero.
    */
    int UnfixPage(int page_id);
    /*
        This function looks at the buffer and returns the number of buffer pages that are free and able to be used. This is especially useful for the N-way sort for the query processor. It returns an integer from 0 to BUFFERSIZE-1(1023).
    */
    int NumFreeFrames();

    // Internal Functions
    /*
        This function selects a frame to replace. If the dirty bit of the selected frame is set then the page needs to be written on to the disk.
    */
    virtual int selectVictim();
    /*
        It takes the page_id as the parameter and returns the frame id.
    */
    int Hash(int page_id);
    /*
        This function removes the Buffer Control Block for the page_id from the array. This is only called if the SelectVictim() function needs to replace a frame.
    */
    void RemoveBCB(BCB * ptr, int page_id);
    /*
        This function removes the LRU element from the list.
    */
    void RemoveLRUEle(int frid);
    /*
        This function sets the dirty bit for the frame_id. This dirty bit is used to know whether or not to write out the frame. A frame must be written if the contents have been modified in any way. This includes any directory pages and data pages. If the bit is 1, it will be written. If this bit is zero, it will not be written.
    */
    void SetDirty(int frame_id);
    /*
        This function assigns the dirty_bit for the corresponding frame_id to zero. The main reason to call this function is when the setDirty() function has been called but the page is actually part of a temporary relation. In this case, the page will not actually need to be written, because it will not want to be saved.
    */
    void UnsetDirty(int frame_id);
    /*
        This function must be called when the system is shut down. The purpose of the function is to write out any pages that are still in the buffer that may need to be written. It will only write pages out to the file if the dirty_bit is one.
    */
    void WriteDirtys();
    /*
        This function prints out the contents of the frame described by the frame_id. 
    */
    void PrintFrame(int frame_id);
private:
    // Hash Table
    int ftop[BUFSIZE];
    BCB* ptof[BUFSIZE];
};

} // namespace DB
            
#endif