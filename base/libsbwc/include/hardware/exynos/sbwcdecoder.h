#include <sys/types.h>

#ifndef __SBWCDECODER_H__
#define __SBWCDECODER_H__

#define SBWCDECODER_ATTR_SECURE_BUFFER  (1 << 0)

class SbwcImgInfo {
public:
    unsigned int fmt;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
};

class SbwcDecoder {
public:
    SbwcDecoder();
    ~SbwcDecoder();
    bool setImage(unsigned int format, unsigned int width,
                  unsigned int height, unsigned int stride);
    bool setImage(unsigned int format, unsigned int width,
                  unsigned int height, unsigned int stride,
                  unsigned int attr, unsigned int framerate = 0);
    bool setImage(SbwcImgInfo &src, SbwcImgInfo &dst, unsigned int dataspace,
                  unsigned int attr, unsigned int framerate = 0);
    bool decode(int inBuf[], size_t inLen[], int outBuf[], size_t outLen[]);
private:
    bool setCtrl();
    bool setFrameRate();
    bool setFmt();
    bool setCrop();
    bool streamOn();
    bool streamOff();
    bool queueBuf(int inBuf[], size_t inLen[], int outBuf[], size_t outLen[]);
    bool dequeueBuf();
    bool reqBufsWithCount(unsigned int count);

    int fd_dev;
    SbwcImgInfo mSrc = {};
    SbwcImgInfo mDst = {};
    unsigned int mSrcNumFd = 0;
    unsigned int mDstNumFd = 0;
    uint32_t mLossyBlockSize = 0;
    bool mIsProtected = 0;
    uint32_t mFrameRate = 0;
    unsigned int mDataspace = 0;
};

#endif
