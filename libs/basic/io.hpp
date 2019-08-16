#ifndef IO_HPP
#define IO_HPP

#include <sys/uio.h>

/**
 * The reader to read data from channel.
 */
class ISrsReader
{
public:
    ISrsReader();
    virtual ~ISrsReader();
public:
    /**
     * Read bytes from reader.
     * @param nread How many bytes read from channel. NULL to ignore.
     */
    virtual int read(void* buf, size_t size, ssize_t* nread) = 0;
};

/**
 * The seeker to seek with a device.
 */
class ISrsSeeker
{
public:
    ISrsSeeker();
    virtual ~ISrsSeeker();
public:
    /**
     * The lseek() function repositions the offset of the file descriptor fildes to the argument offset, according to the
     * directive whence. lseek() repositions the file pointer fildes as follows:
     *      If whence is SEEK_SET, the offset is set to offset bytes.
     *      If whence is SEEK_CUR, the offset is set to its current location plus offset bytes.
     *      If whence is SEEK_END, the offset is set to the size of the file plus offset bytes.
     * @param seeked Upon successful completion, lseek() returns the resulting offset location as measured in bytes from
     *      the beginning of the file. NULL to ignore.
     */
    virtual int lseek(off_t offset, int whence, off_t* seeked) = 0;
};

/**
 * The reader and seeker.
 */
class ISrsReadSeeker : virtual public ISrsReader, virtual public ISrsSeeker
{
public:
    ISrsReadSeeker();
    virtual ~ISrsReadSeeker();
};

/**
 * The writer to write stream data to channel.
 */
class ISrsStreamWriter
{
public:
    ISrsStreamWriter();
    virtual ~ISrsStreamWriter();
public:
    /**
     * write bytes over writer.
     * @nwrite the actual written bytes. NULL to ignore.
     */
    virtual int write(void* buf, size_t size, ssize_t* nwrite) = 0;
};

/**
 * The vector writer to write vector(iovc) to channel.
 */
class ISrsVectorWriter
{
public:
    ISrsVectorWriter();
    virtual ~ISrsVectorWriter();
public:
    /**
     * write iov over writer.
     * @nwrite the actual written bytes. NULL to ignore.
     * @remark for the HTTP FLV, to writev to improve performance.
     *      @see https://github.com/ossrs/srs/issues/405
     */
    virtual int writev(const iovec *iov, int iov_size, ssize_t* nwrite) = 0;
};

/**
 * The generally writer, stream and vector writer.
 */
class ISrsWriter : virtual public ISrsStreamWriter, virtual public ISrsVectorWriter
{
public:
    ISrsWriter();
    virtual ~ISrsWriter();
};

/**
 * The writer and seeker.
 */
class ISrsWriteSeeker : virtual public ISrsWriter, virtual public ISrsSeeker
{
public:
    ISrsWriteSeeker();
    virtual ~ISrsWriteSeeker();
};

#endif