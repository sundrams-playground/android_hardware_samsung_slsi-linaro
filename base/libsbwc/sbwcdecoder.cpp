#include <cstring>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include <log/log.h>
#include <system/graphics.h>

#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>

#include <hardware/exynos/sbwcdecoder.h>

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

#define MSCLPATH "/dev/video50"

#ifndef ALOGERR
#define ALOGERR(fmt, args...) ((void)ALOG(LOG_ERROR, LOG_TAG, fmt " [%s]", ##args, strerror(errno)))
#endif

#define ARRSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define V4L2_PIX_FMT_NV12N             v4l2_fourcc('N', 'N', '1', '2')
#define V4L2_PIX_FMT_NV12M_P010        v4l2_fourcc('P', 'M', '1', '2')
/* 12 Y/CbCr 4:2:0 SBWC */
#define V4L2_PIX_FMT_NV12M_SBWC_8B     v4l2_fourcc('M', '1', 'S', '8')
#define V4L2_PIX_FMT_NV12M_SBWC_10B    v4l2_fourcc('M', '1', 'S', '1')
/* 21 Y/CrCb 4:2:0 SBWC */
#define V4L2_PIX_FMT_NV21M_SBWC_8B     v4l2_fourcc('M', '2', 'S', '8')
#define V4L2_PIX_FMT_NV21M_SBWC_10B    v4l2_fourcc('M', '2', 'S', '1')
/* 12 Y/CbCr 4:2:0 SBWC single */
#define V4L2_PIX_FMT_NV12N_SBWC_8B     v4l2_fourcc('N', '1', 'S', '8')
#define V4L2_PIX_FMT_NV12N_SBWC_10B    v4l2_fourcc('N', '1', 'S', '1')
/* 12 Y/CbCr 4:2:0 SBWC Lossy */
#define V4L2_PIX_FMT_NV12M_SBWCL_8B    v4l2_fourcc('M', '1', 'L', '8')
#define V4L2_PIX_FMT_NV12M_SBWCL_10B   v4l2_fourcc('M', '1', 'L', '1')

#define EXYNOS_CID_BASE             (V4L2_CTRL_CLASS_USER| 0x2000U)
#define V4L2_CID_CONTENT_PROTECTION (EXYNOS_CID_BASE + 201)
#define SC_CID_FRAMERATE            (EXYNOS_CID_BASE + 110)

enum {
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M      = 0x105,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN       = 0x123,
    /* 10-bit format (2 fd, 10bit, 2x byte) custom formats */
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M        = 0x127,
    /* SBWC format */
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC = 0x130,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC  = 0x131,

    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC = 0x132,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC  = 0x133,

    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC = 0x134,

    /* SBWC Lossy formats */
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50 = 0x140,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75 = 0x141,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50  = 0x150,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75  = 0x151,

    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40 = 0x160,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60 = 0x161,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80 = 0x162,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40  = 0x170,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60  = 0x171,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80  = 0x172,
};

SbwcDecoder::SbwcDecoder()
{
    fd_dev = open(MSCLPATH, O_RDWR);
    if (fd_dev < 0) {
        ALOGERR("Failed to open %s", MSCLPATH);
        return;
    }
}

SbwcDecoder::~SbwcDecoder()
{
    if (fd_dev >= 0)
        close(fd_dev);
}

bool SbwcDecoder::reqBufsWithCount(unsigned int count)
{
    ATRACE_CALL();

    v4l2_requestbuffers reqbufs;

    reqbufs.count = count;
    reqbufs.memory = V4L2_MEMORY_DMABUF;

    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (ioctl(fd_dev, VIDIOC_REQBUFS, &reqbufs) < 0) {
        ALOGERR("Failed to REQBUFS(SRC)");
        return false;
    }

    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(fd_dev, VIDIOC_REQBUFS, &reqbufs) < 0) {
        ALOGERR("Failed to REQBUFS(DST)");
        return false;
    }

    return true;
}

static struct {
    unsigned int hal;
    uint32_t v4l2;
} __halDataspaceToV4L2[] = {
    {HAL_DATASPACE_STANDARD_BT709,      V4L2_COLORSPACE_REC709},
    {HAL_DATASPACE_STANDARD_BT601_625,  V4L2_COLORSPACE_SMPTE170M},
    {HAL_DATASPACE_STANDARD_BT601_525,  V4L2_COLORSPACE_SMPTE170M},
    {HAL_DATASPACE_STANDARD_BT2020,     V4L2_COLORSPACE_BT2020},
    {HAL_DATASPACE_STANDARD_FILM,       V4L2_COLORSPACE_REC709},
    {HAL_DATASPACE_SRGB,                V4L2_COLORSPACE_REC709},
    {HAL_DATASPACE_JFIF,                V4L2_COLORSPACE_SMPTE170M},
    {HAL_DATASPACE_BT601_525,           V4L2_COLORSPACE_SMPTE170M},
    {HAL_DATASPACE_BT601_625,           V4L2_COLORSPACE_SMPTE170M},
    {HAL_DATASPACE_BT709,               V4L2_COLORSPACE_REC709},
};

#define HAL_DATASPACE_LEGACY_TYPE_MASK  ((1 << HAL_DATASPACE_STANDARD_SHIFT) - 1)

uint32_t halDataspaceToV4L2(unsigned int dataspace, unsigned int width, unsigned int height)
{
    if ((dataspace & HAL_DATASPACE_LEGACY_TYPE_MASK) != 0) {
        dataspace &= HAL_DATASPACE_LEGACY_TYPE_MASK;
    } else {
        dataspace &= ~HAL_DATASPACE_RANGE_MASK;
        dataspace &= ~HAL_DATASPACE_TRANSFER_MASK;

        if ((dataspace & HAL_DATASPACE_STANDARD_MASK) == 0) {
            dataspace |= ((width * height) < (1280 * 720)) ? HAL_DATASPACE_STANDARD_BT601_625
                                                           : HAL_DATASPACE_STANDARD_BT709;
        }
    }

    for (int i = 0; i < ARRSIZE(__halDataspaceToV4L2); i++) {
        if (dataspace == __halDataspaceToV4L2[i].hal)
            return __halDataspaceToV4L2[i].v4l2;
    }

    return V4L2_COLORSPACE_DEFAULT;
}

#define V4L2_CID_CSC_EQ             (EXYNOS_CID_BASE + 101)
#define V4L2_CID_CSC_RANGE          (EXYNOS_CID_BASE + 102)
#define DATASPACE_RANGE_FULL        1
#define DATASPACE_RANGE_LIMITED     0
#define isRGB(fmt)  (fmt == V4L2_PIX_FMT_RGB32)
bool SbwcDecoder::setCtrl()
{
    ATRACE_CALL();

    v4l2_control ctrl;

    ctrl.id = V4L2_CID_CONTENT_PROTECTION;
    ctrl.value = mIsProtected;

    if (ioctl(fd_dev, VIDIOC_S_CTRL, &ctrl) < 0) {
        ALOGERR("Failed to S_CTRL to make [%s]",
                (mIsProtected) ? "SECURE mode" : "NORMAL mode");
        return false;
    }

    if (isRGB(mDst.fmt)) {
        ctrl.id = V4L2_CID_CSC_EQ;
        ctrl.value = halDataspaceToV4L2(mDataspace, mSrc.width, mSrc.height);
        if (ioctl(fd_dev, VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to set csc_dataspace to %d", ctrl.value);
            return false;
        }

        ctrl.id = V4L2_CID_CSC_RANGE;
        ctrl.value = (mDataspace & HAL_DATASPACE_RANGE_FULL) ? DATASPACE_RANGE_FULL
                                                             : DATASPACE_RANGE_LIMITED;
        if (ioctl(fd_dev, VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to set csc_range to %s",
                    (ctrl.value == DATASPACE_RANGE_FULL) ? "WIDE" : "NARROW");
            return false;
        }
    }

    return true;
}

bool SbwcDecoder::setFrameRate()
{
    ATRACE_CALL();

    v4l2_control ctrl;

    ctrl.id = SC_CID_FRAMERATE;
    ctrl.value = mFrameRate;

    if (ioctl(fd_dev, VIDIOC_S_CTRL, &ctrl) < 0) {
        ALOGERR("Failed to S_CTRL to set framerate %d", mFrameRate);
        return false;
    }

    return true;
}

#define ALIGN_SBWC(val) (((val) + 31) & ~31)

bool SbwcDecoder::setFmt()
{
    ATRACE_CALL();

    v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));

    fmt.fmt.pix_mp.height = mSrc.height;
    // TODO : how to get colorspace, but MSCL driver doesn't use this.
    //fmt.fmt.pix_mp.colorspace = haldataspace_to_v4l2(canvas.getDataspace(), coord.hori, coord.vert);
    fmt.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_DEFAULT;
    fmt.fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_DEFAULT;

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.width = mSrc.stride;
    fmt.fmt.pix_mp.pixelformat = mSrc.fmt;
    fmt.fmt.pix_mp.flags = mLossyBlockSize;

    if (ioctl(fd_dev, VIDIOC_S_FMT, &fmt) < 0) {
        ALOGERR("Failed to S_FMT(SRC):width(%d), height(%d), fmt(%#x)",
                mSrc.stride, mSrc.height, mSrc.fmt);
        return false;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = mDst.stride;
    fmt.fmt.pix_mp.pixelformat = mDst.fmt;
    fmt.fmt.pix_mp.flags = 0;

    if (ioctl(fd_dev, VIDIOC_S_FMT, &fmt) < 0) {
        ALOGERR("Failed to S_FMT(DST):width(%d), height(%d), fmt(%#x)",
                mDst.stride, mDst.height, mDst.fmt);
        return false;
    }

    return true;
}

bool SbwcDecoder::setCrop()
{
    ATRACE_CALL();

    v4l2_crop crop;

    crop.c.left = 0;
    crop.c.top = 0;

    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    crop.c.width = mSrc.width;
    crop.c.height = mSrc.height;

    if (ioctl(fd_dev, VIDIOC_S_CROP, &crop) < 0) {
        ALOGERR("Failed to S_CROP(SRC):width(%d), height(%d)", mSrc.width, mSrc.height);
        return false;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    crop.c.width = mDst.width;
    crop.c.height = mDst.height;

    if (ioctl(fd_dev, VIDIOC_S_CROP, &crop) < 0) {
        ALOGERR("Failed to S_CROP(DST):width(%d), height(%d)", mDst.width, mDst.height);
        return false;
    }

    return true;
}

bool SbwcDecoder::streamOn()
{
    ATRACE_CALL();

    enum v4l2_buf_type bufType;

    bufType = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (ioctl(fd_dev, VIDIOC_STREAMON, &bufType) < 0) {
        ALOGERR("Failed to STREAMON(SRC)");
        return false;
    }

    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd_dev, VIDIOC_STREAMON, &bufType) < 0) {
        ALOGERR("Failed to STREAMON(DST)");
        return false;
    }

    return true;
}

bool SbwcDecoder::streamOff()
{
    ATRACE_CALL();

    enum v4l2_buf_type bufType;

    bufType = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (ioctl(fd_dev, VIDIOC_STREAMOFF, &bufType) < 0) {
        ALOGERR("Failed to STREAMOFF(SRC)");
        return false;
    }

    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd_dev, VIDIOC_STREAMOFF, &bufType) < 0) {
        ALOGERR("Failed to STREAMOFF(DST)");
        return false;
    }

    return true;
}

//TODO : data_offset is not set, calculate byteused
bool SbwcDecoder::queueBuf(int inBuf[], size_t inLen[],
                           int outBuf[], size_t outLen[])
{
    ATRACE_CALL();

    v4l2_buffer buffer;
    v4l2_plane planes[4];

    memset(&buffer, 0, sizeof(buffer));

    buffer.memory = V4L2_MEMORY_DMABUF;

    memset(planes, 0, sizeof(planes));

    buffer.length = mSrcNumFd;
    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    for (unsigned int i = 0; i < mSrcNumFd; i++) {
        planes[i].length = static_cast<unsigned int>(inLen[i]);
        //planes[i].bytesused = ;
        planes[i].m.fd = inBuf[i];
        //planes[i].data_offset = ;
    }
    buffer.m.planes = planes;

    if (ioctl(fd_dev, VIDIOC_QBUF, &buffer) < 0) {
        ALOGERR("Failed to QBUF(SRC)");
        return false;
    }

    memset(planes, 0, sizeof(planes));

    buffer.length = mDstNumFd;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    for (unsigned int i = 0; i < mDstNumFd; i++) {
        planes[i].length = static_cast<unsigned int>(outLen[i]);
        planes[i].m.fd = outBuf[i];
        //planes[i].data_offset = ;
    }
    buffer.m.planes = planes;

    if (ioctl(fd_dev, VIDIOC_QBUF, &buffer) < 0) {
        ALOGERR("Failed to QBUF(DST)");
        return false;
    }

    return true;
}

bool SbwcDecoder::dequeueBuf()
{
    ATRACE_CALL();

    v4l2_buffer buffer;
    v4l2_plane planes[4];

    memset(&buffer, 0, sizeof(buffer));

    buffer.memory = V4L2_MEMORY_DMABUF;

    buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    memset(planes, 0, sizeof(planes));
    buffer.length = 4;
    buffer.m.planes = planes;

    if (ioctl(fd_dev, VIDIOC_DQBUF, &buffer) < 0) {
        ALOGERR("Failed to DQBUF(SRC)");
        return false;
    } else if (!!(buffer.flags & V4L2_BUF_FLAG_ERROR)) {
        ALOGERR("%s:Error during running(SRC)", __func__);
        return false;
    }

    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    memset(planes, 0, sizeof(planes));
    buffer.length = 4;
    buffer.m.planes = planes;

    if (ioctl(fd_dev, VIDIOC_DQBUF, &buffer) < 0) {
        ALOGERR("Failed to DQBUF(DST)");
        return false;
    } else if (!!(buffer.flags & V4L2_BUF_FLAG_ERROR)) {
        ALOGERR("%s:Error during running(DST)", __func__);
        return false;
    }

    return true;
}

bool SbwcDecoder::decode(int inBuf[], size_t inLen[],
                         int outBuf[], size_t outLen[])
{
    bool ret;

    ret = setCtrl();
    if (ret)
        ret = setFmt();
    if (ret)
        ret = setCrop();
    if (ret)
        ret = setFrameRate();
    if (ret)
        ret = reqBufsWithCount(1);
    if (ret)
        ret = streamOn();
    if (ret)
        ret = queueBuf(inBuf, inLen, outBuf, outLen);
    if (ret)
        ret = dequeueBuf();

    streamOff();
    reqBufsWithCount(0);

    return ret;
}

static struct {
    uint32_t halFmtSBWC;
    uint32_t halFmtNonSBWC;
} __halfmtSBWC_to_halfmtNonSBWC[] = {
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,       HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,       HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,        HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80,   HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,            HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,              HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M },
};

static struct {
    uint32_t fmtHal;
    uint32_t fmtV4L2;
    uint32_t numFd;
    uint32_t blockSz;
} __halfmtSBWC_to_v4l2[] = {
    // { (HAL format), (V4L2 format), (num fd), (block size of sbwc lossy) }
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,       V4L2_PIX_FMT_NV12M_SBWC_8B,  2, 0 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,   V4L2_PIX_FMT_NV12M_SBWC_10B, 2, 0 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,       V4L2_PIX_FMT_NV21M_SBWC_8B,  2, 0 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,        V4L2_PIX_FMT_NV12N_SBWC_8B,  1, 0 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,    V4L2_PIX_FMT_NV12N_SBWC_10B, 1, 0 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,   V4L2_PIX_FMT_NV12M_SBWCL_8B, 2, 64},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75,   V4L2_PIX_FMT_NV12M_SBWCL_8B, 2, 96},
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40, V4L2_PIX_FMT_NV12M_SBWCL_10B, 2, 64 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60, V4L2_PIX_FMT_NV12M_SBWCL_10B, 2, 96 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80, V4L2_PIX_FMT_NV12M_SBWCL_10B, 2, 128},
};

static struct {
    uint32_t fmtHal;
    uint32_t fmtV4L2;
    uint32_t numFd;
} __halfmtNonSBWC_to_v4l2[] = {
    // { (HAL format), (V4L2 format), (num fd) }
    {HAL_PIXEL_FORMAT_YCBCR_420_888,            V4L2_PIX_FMT_NV12N,      1 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,     V4L2_PIX_FMT_NV12N,      1 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,    V4L2_PIX_FMT_NV12M,      2 },
    {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,      V4L2_PIX_FMT_NV12M_P010, 2 },
    {HAL_PIXEL_FORMAT_RGBA_8888,                V4L2_PIX_FMT_RGB32,      1 },
};

bool SbwcDecoder::setImage(unsigned int format, unsigned int width,
                           unsigned int height, unsigned int stride)
{
    return setImage(format, width, height, stride, 0);
}

bool SbwcDecoder::setImage(unsigned int format, unsigned int width,
                           unsigned int height, unsigned int stride,
                           unsigned int attr, unsigned int framerate)
{
    unsigned int dstFmt = 0;

    for (size_t i = 0; i < ARRSIZE(__halfmtSBWC_to_halfmtNonSBWC); i++) {
        if (format == __halfmtSBWC_to_halfmtNonSBWC[i].halFmtSBWC) {
            dstFmt = __halfmtSBWC_to_halfmtNonSBWC[i].halFmtNonSBWC;

            break;
        }
    }

    if (dstFmt == 0) {
        ALOGE("Unable to find the proper v4l2 format for HAL format(SBWC) %#x", format);
        return false;
    }

    SbwcImgInfo src{format, width, height, ALIGN_SBWC(stride)};
    SbwcImgInfo dst{dstFmt, width, height, stride};

    return setImage(src, dst, 0, attr, framerate);
}

bool SbwcDecoder::setImage(SbwcImgInfo &src, SbwcImgInfo &dst,
                           unsigned int dataspace, unsigned int attr, unsigned int framerate)
{
    ATRACE_CALL();

    mSrc.fmt = 0;
    for (size_t i = 0; i < ARRSIZE(__halfmtSBWC_to_v4l2); i++) {
        if (src.fmt == __halfmtSBWC_to_v4l2[i].fmtHal) {
            mSrc.fmt = __halfmtSBWC_to_v4l2[i].fmtV4L2;
            mSrcNumFd = __halfmtSBWC_to_v4l2[i].numFd;
            mLossyBlockSize = __halfmtSBWC_to_v4l2[i].blockSz;

            break;
        }
    }
    if (mSrc.fmt == 0) {
        for (size_t i = 0; i < ARRSIZE(__halfmtNonSBWC_to_v4l2); i++) {
            if (src.fmt == __halfmtNonSBWC_to_v4l2[i].fmtHal) {
                mSrc.fmt = __halfmtNonSBWC_to_v4l2[i].fmtV4L2;
                mSrcNumFd = __halfmtNonSBWC_to_v4l2[i].numFd;
                mLossyBlockSize = 0;

                break;
            }
        }
    }
    if (mSrc.fmt == 0) {
        ALOGE("fail to find the proper v4l2 format for HAL format(SRC) %#x", mSrc.fmt);
        return false;
    }

    mDst.fmt = 0;
    for (size_t i = 0; i < ARRSIZE(__halfmtNonSBWC_to_v4l2); i++) {
        if (dst.fmt == __halfmtNonSBWC_to_v4l2[i].fmtHal) {
            mDst.fmt = __halfmtNonSBWC_to_v4l2[i].fmtV4L2;
            mDstNumFd = __halfmtNonSBWC_to_v4l2[i].numFd;

            break;
        }
    }
    if (mDst.fmt == 0) {
        ALOGE("fail to find the proper v4l2 format for HAL format(DST) %#x", mDst.fmt);
        return false;
    }

    mSrc.width = src.width;
    mSrc.height = src.height;
    mSrc.stride = src.stride;

    mDst.width = dst.width;
    mDst.height = dst.height;
    mDst.stride = dst.stride;

    mDataspace = dataspace;
    mFrameRate = framerate;
    mIsProtected = !!(attr & SBWCDECODER_ATTR_SECURE_BUFFER);

    return true;
}
