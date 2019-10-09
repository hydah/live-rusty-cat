
#ifndef FILE_HPP
#define FILE_HPP

#include <sys/uio.h>
#include "rtmp_tool.h"
#include <string>
#include "io.hpp"

/**
* file writer, to write to file.
*/
class SrsFileWriter : public ISrsWriteSeeker
{
private:
    std::string path;
    int fd;
public:
    SrsFileWriter();
    virtual ~SrsFileWriter();
public:
    /**
     * open file writer, in truncate mode.
     * @param p a string indicates the path of file to open.
     */
    virtual int open(std::string p);
    /**
     * open file writer, in append mode.
     * @param p a string indicates the path of file to open.
     */
    virtual int open_append(std::string p);
    /**
     * close current writer.
     * @remark user can reopen again.
     */
    virtual void close();
public:
    virtual bool is_open();
    virtual void seek2(int64_t offset);
    virtual int64_t tellg();
// Interface ISrsWriteSeeker
public:
    virtual int write(void* buf, size_t count, ssize_t* pnwrite);
    virtual int writev(const iovec* iov, int iovcnt, ssize_t* pnwrite);
    virtual int lseek(off_t offset, int whence, off_t* seeked);
};


/**
* file reader, to read from file.
*/
class SrsFileReader : public ISrsReadSeeker
{
private:
    std::string path;
    int fd;
public:
    SrsFileReader();
    virtual ~SrsFileReader();
public:
    /**
     * open file reader.
     * @param p a string indicates the path of file to open.
     */
    virtual int open(std::string p);
    /**
     * close current reader.
     * @remark user can reopen again.
     */
    virtual void close();
public:
    // TODO: FIXME: extract interface.
    virtual bool is_open();
    virtual int64_t tellg();
    virtual void skip(int64_t size);
    virtual int64_t seek2(int64_t offset);
    virtual int64_t filesize();
// Interface ISrsReadSeeker
public:
    virtual int read(void* buf, size_t count, ssize_t* pnread);
    virtual int lseek(off_t offset, int whence, off_t* seeked);
};

#endif