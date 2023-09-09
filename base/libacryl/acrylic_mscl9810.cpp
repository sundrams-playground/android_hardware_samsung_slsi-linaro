#include <cstring>

#include <log/log.h>

#include <linux/v4l2-controls.h>

#include "acrylic_internal.h"
#include "acrylic_mscl9810.h"

#ifndef V4L2_BUF_FLAG_USE_SYNC
#define V4L2_BUF_FLAG_USE_SYNC         0x00008000
#endif

#define V4L2_BUF_FLAG_IN_FENCE         0x00200000
#define V4L2_BUF_FLAG_OUT_FENCE        0x00400000

#define V4L2_CAP_FENCES                0x20000000
#define SC_V4L2_CAP_VOTF               0x40000000

#define EXYNOS_CID_BASE             (V4L2_CTRL_CLASS_USER| 0x2000U)
#define V4L2_CID_CONTENT_PROTECTION (EXYNOS_CID_BASE + 201)
#define V4L2_CID_CSC_EQ             (EXYNOS_CID_BASE + 101)
#define V4L2_CID_CSC_RANGE          (EXYNOS_CID_BASE + 102)
#define SC_CID_FRAMERATE            (EXYNOS_CID_BASE + 110)

#define MAX_ARG_PPC_IOCTL   10
struct sc_ioctl_ppc_arg {
    struct {
        uint32_t v4l2fmt;
        uint32_t ppc;
        uint32_t ppc_rot;
    } elem[MAX_ARG_PPC_IOCTL];
};
#define SC_CMD_G_PPC	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct sc_ioctl_ppc_arg)

static const char *__dirname[AcrylicCompositorMSCL9810::NUM_IMAGES] = {"source", "target"};

AcrylicCompositorMSCL9810::AcrylicCompositorMSCL9810(const HW2DCapability &capability)
    : Acrylic(capability), mDev("/dev/video50"), mCurrentTransform(0), mProtectedContent(false),
      mTransformChanged(false), mCurrentPixFmt{0, 0},
      mCurrentTypeBuf{V4L2_BUF_TYPE_VIDEO_OUTPUT, V4L2_BUF_TYPE_VIDEO_CAPTURE},
      mCurrentTypeMem{0, 0}, mDeviceState{0, 0}, mUseFenceFlag(V4L2_BUF_FLAG_USE_SYNC)
{
    memset(&mCurrentCrop, 0, sizeof(mCurrentCrop));

    v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (mDev.ioctl(VIDIOC_QUERYCAP, &cap) == 0) {
        if (cap.device_caps & V4L2_CAP_FENCES)
            mUseFenceFlag = V4L2_BUF_FLAG_IN_FENCE | V4L2_BUF_FLAG_OUT_FENCE;
        if (cap.device_caps & SC_V4L2_CAP_VOTF)
            mVotfSupported = true;
    }

    struct sc_ioctl_ppc_arg ppcTable;
    memset(&ppcTable, 0, sizeof(ppcTable));
    if (mDev.ioctl(SC_CMD_G_PPC, &ppcTable) == 0) {
        for (int i = 0; i < 10; i++) {
            if (ppcTable.elem[i].v4l2fmt == 0)
                break;

            uint32_t halFmt;

            halFmt = v4l2_deprecated_to_halfmt(ppcTable.elem[i].v4l2fmt);
            if (halFmt != 0)
                pushPPC(halFmt, ppcTable.elem[i].ppc, ppcTable.elem[i].ppc_rot);
        }
    }
}

AcrylicCompositorMSCL9810::~AcrylicCompositorMSCL9810()
{
}

bool AcrylicCompositorMSCL9810::resetMode(AcrylicCanvas &canvas, BUFDIRECTION dir)
{
    if (testDeviceState(dir, STATE_REQBUFS)) {
        v4l2_requestbuffers reqbufs;

        reqbufs.count = 0;
        reqbufs.type = mCurrentTypeBuf[dir];
        reqbufs.memory = (mCurrentTypeMem[dir] == AcrylicCanvas::MT_DMABUF) ? V4L2_MEMORY_DMABUF
                                                                            : V4L2_MEMORY_USERPTR;
        if (mDev.ioctl(VIDIOC_STREAMOFF, &reqbufs.type) < 0) {
            hw2d_coord_t coord = canvas.getImageDimension();
            ALOGERR("Failed to streamoff to reset format of %s to %#x/%dx%d but forcing reqbufs(0)",
                    __dirname[dir], canvas.getFormat(), coord.hori, coord.vert);
        }

        ALOGD_TEST("VIDIOC_STREAMOFF: type=%d", reqbufs.type);

        if (mDev.ioctl(VIDIOC_REQBUFS, &reqbufs) < 0) {
            hw2d_coord_t coord = canvas.getImageDimension();
            ALOGERR("Failed to reqbufs(0) to reset format of %s to %#x/%dx%d",
                    __dirname[dir], canvas.getFormat(), coord.hori, coord.vert);
            return false;
        }

        ALOGD_TEST("VIDIOC_REQBUFS: count=%d, type=%d, memory=%d", reqbufs.count, reqbufs.type, reqbufs.memory);

        clearDeviceState(dir, STATE_REQBUFS);
    }

    return true;
}

bool AcrylicCompositorMSCL9810::resetMode()
{
    bool reset_required = false;
    hw2d_rect_t crop = getLayer(0)->getTargetRect();
    int mod_flags = AcrylicCanvas::SETTING_TYPE_MODIFIED |
                    AcrylicCanvas::SETTING_DIMENSION_MODIFIED |
                    AcrylicCanvas::SETTING_STRIDE_MODIFIED;

    // If crop size, dimension, format is changed in any direction,
    // MSCL driver needs reqbufs(0) to the both directions.
    if (area_is_zero(crop))
        crop.size = getCanvas().getImageDimension();

    if ((crop != mCurrentCrop[TARGET]) || (getLayer(0)->getImageRect() != mCurrentCrop[SOURCE]))
        reset_required = true;

    if (((getLayer(0)->getSettingFlags() | getCanvas().getSettingFlags()) & mod_flags) != 0)
        reset_required = true;

    if (getCanvas().isProtected() != mProtectedContent)
        reset_required = true;

    if (mTransformChanged) {
        reset_required = true;
        mTransformChanged = false;
    }

    if (reset_required) {
        // Ignore the return value because we have no choice when it is false.
        resetMode(*getLayer(0), SOURCE);
        resetMode(getCanvas(), TARGET);

        // It is alright to configure CSC before S_FMT.
        if (!configureCSC())
            return false;
    }

    return true;
}

bool AcrylicCompositorMSCL9810::changeMode(AcrylicCanvas &canvas, BUFDIRECTION dir)
{
    if (testDeviceState(dir, STATE_REQBUFS))
        return true;

    v4l2_buf_type buftype = (dir == SOURCE) ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
                                            : V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (!setFormat(canvas, buftype))
        return false;

    mCurrentPixFmt[dir] = canvas.getFormat();
    mCurrentCrop[dir].size = canvas.getImageDimension();
    mCurrentCrop[dir].pos = {0, 0};

    hw2d_rect_t rect;
    if (dir == SOURCE) {
        rect = getLayer(0)->getImageRect();
    } else {
        rect = getLayer(0)->getTargetRect();
        if (area_is_zero(rect))
            rect.size = getCanvas().getImageDimension();
    }

    if (!setCrop(rect, buftype, mCurrentCrop[dir]))
        return false;

    return true;
}

bool AcrylicCompositorMSCL9810::setFormat(AcrylicCanvas &canvas, v4l2_buf_type buftype)
{
    hw2d_coord_t coord = canvas.getImageDimension();
    uint32_t pixfmt = halfmt_to_v4l2_deprecated(canvas.getFormat());
    uint8_t blocksize = get_block_size_from_halfmt(canvas.getFormat());
    v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));

    // S_FMT always successes unless type is invalid.
    fmt.type = buftype;

    fmt.fmt.pix_mp.width = coord.hori;
    fmt.fmt.pix_mp.height = coord.vert;
    fmt.fmt.pix_mp.pixelformat = pixfmt;
    fmt.fmt.pix_mp.flags = blocksize;
    fmt.fmt.pix_mp.colorspace = haldataspace_to_v4l2(canvas.getDataspace(), coord.hori, coord.vert);
    fmt.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    fmt.fmt.pix_mp.quantization = V4L2_QUANTIZATION_DEFAULT;
    fmt.fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_DEFAULT;

    for (int i = 0; i < MAX_HW2D_PLANES; i++)
        fmt.fmt.pix_mp.plane_fmt[i].bytesperline = canvas.getStride(i);

    ALOGD_TEST("VIDIOC_S_FMT: v4l2_fmt/mp .type=%d, .width=%d, .height=%d, .pixelformat=%#x, .colorspace=%d\n"
               "                          .ycbcr_enc=%d, quantization=%d, .xfer_func=%d",
               fmt.type, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height, fmt.fmt.pix_mp.pixelformat,
               fmt.fmt.pix_mp.colorspace, fmt.fmt.pix_mp.ycbcr_enc, fmt.fmt.pix_mp.quantization,
               fmt.fmt.pix_mp.xfer_func);

    if (mDev.ioctl(VIDIOC_S_FMT, &fmt) < 0) {
        ALOGERR("Failed VIDIOC_S_FMT .type=%d, .width=%d, .height=%d, .pixelformat=%#x",
                fmt.type, fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
        return false;
    }

    return true;
}

bool AcrylicCompositorMSCL9810::setTransform()
{
    uint32_t trdiff = mCurrentTransform ^ getLayer(0)->getTransform();
    v4l2_control ctrl;

    mTransformChanged = !!trdiff;

    // TODO: consider to use rot 180 and 270
    if (trdiff & HAL_TRANSFORM_FLIP_H) {
        ctrl.id = V4L2_CID_HFLIP;
        ctrl.value = !!(getLayer(0)->getTransform() & HAL_TRANSFORM_FLIP_H);
        ALOGD_TEST("VIDIOC_S_CTRL: HFLIP=%d, (transform=%d)", ctrl.value, getLayer(0)->getTransform());
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure HFLIP to %d", ctrl.value);
            return false;
        }
        trdiff &= ~HAL_TRANSFORM_FLIP_H;
    }

    if (trdiff & HAL_TRANSFORM_FLIP_V) {
        ctrl.id = V4L2_CID_VFLIP;
        ctrl.value = !!(getLayer(0)->getTransform() & HAL_TRANSFORM_FLIP_V);
        ALOGD_TEST("VIDIOC_S_CTRL: VFLIP=%d, (transform=%d)", ctrl.value, getLayer(0)->getTransform());
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure VFLIP to %d", ctrl.value);
            return false;
        }
        trdiff &= ~HAL_TRANSFORM_FLIP_V;
    }

    if (trdiff & HAL_TRANSFORM_ROT_90) {
        ctrl.id = V4L2_CID_ROTATE;
        ctrl.value = !(getLayer(0)->getTransform() & HAL_TRANSFORM_ROT_90) ? 0 : 90;
        ALOGD_TEST("VIDIOC_S_CTRL: ROTATE=%d, (transform=%d)", ctrl.value, getLayer(0)->getTransform());
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure Rotation of 90");
            return false;
        }
        trdiff &= ~HAL_TRANSFORM_ROT_90;
    }

    LOGASSERT(trdiff == 0, "Unexpected transform option is changed: %#x", trdiff);

    mCurrentTransform = getLayer(0)->getTransform();

    return true;
}

bool AcrylicCompositorMSCL9810::setCrop(hw2d_rect_t rect, v4l2_buf_type buftype, hw2d_rect_t &save_rect)
{
    v4l2_crop crop;

    crop.type = buftype;
    crop.c.left = rect.pos.hori;
    crop.c.top = rect.pos.vert;
    crop.c.width = rect.size.hori;
    crop.c.height = rect.size.vert;
    ALOGD_TEST("VIDIOC_S_CROP: type=%d, left=%d, top=%d, width=%d, height=%d",
            buftype, crop.c.left, crop.c.top, crop.c.width, crop.c.height);
    if (mDev.ioctl(VIDIOC_S_CROP, &crop) < 0) {
        ALOGERR("Failed to set crop of type %d to %dx%d@%dx%d", buftype,
                rect.size.hori, rect.size.vert, rect.pos.hori, rect.pos.vert);
        return false;
    }

    save_rect = rect;

    return true;
}

bool AcrylicCompositorMSCL9810::prepareExecute(AcrylicCanvas &canvas, BUFDIRECTION dir)
{
    if (testDeviceState(dir, STATE_REQBUFS))
        return true;

    v4l2_buf_type buftype = (dir == SOURCE) ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
                                            : V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_requestbuffers reqbufs;

    reqbufs.count = 1;
    reqbufs.type = buftype;
    reqbufs.memory = (canvas.getBufferType() == AcrylicCanvas::MT_DMABUF)
                     ? V4L2_MEMORY_DMABUF : V4L2_MEMORY_USERPTR;

    ALOGD_TEST("VIDIOC_REQBUFS: v4l2_requestbuffers .count=%d, .type=%d, .memory=%d, .reserved={%d,%d}",
               reqbufs.count, reqbufs.type, reqbufs.memory, reqbufs.reserved[0], reqbufs.reserved[1]);

    if (mDev.ioctl(VIDIOC_REQBUFS, &reqbufs)) {
        ALOGERR("Failed VIDIOC_REQBUFS: count=%d, type=%d, memory=%d", reqbufs.count, reqbufs.type, reqbufs.memory);
        return false;
    }

    if (mDev.ioctl(VIDIOC_STREAMON, &reqbufs.type) < 0) {
        ALOGERR("Failed VIDIOC_STREAMON with type %d", reqbufs.type);
        // we don't need to cancel the previous s_fmt and reqbufs
        // because we will try it again for the next frame.
        reqbufs.count = 0;
        mDev.ioctl(VIDIOC_REQBUFS, &reqbufs); // cancel reqbufs. ignore result
        return false;
    }

    ALOGD_TEST("VIDIOC_STREAMON: type=%d", reqbufs.type);

    mCurrentTypeMem[dir] = canvas.getBufferType();
    mCurrentTypeBuf[dir] = buftype;

    setDeviceState(dir, STATE_REQBUFS);

    return true;
}

bool AcrylicCompositorMSCL9810::prepareExecute()
{
    if (getCanvas().isProtected() != mProtectedContent) {
        v4l2_control ctrl;

        ctrl.id = V4L2_CID_CONTENT_PROTECTION;
        ctrl.value = getCanvas().isProtected();
        ALOGD_TEST("VIDIOC_S_CTRL: V4L2_CID_CONTENT_PROTECTION=%d", ctrl.value);
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure content protection to %d", ctrl.value);
            return false;
        }

        mProtectedContent = getCanvas().isProtected();
    }

    if (mFramerateChanged) {
        v4l2_control ctrl;

        ctrl.id = SC_CID_FRAMERATE;
        ctrl.value = mFramerate;

        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            /*
             * It doesn't return EINVAL by value.
             * But in case of not supporting SC_CID_FRAMERATE it returns EINVAL.
             * Some chips don't support this feature.So, just keep running.
             */
            if (errno != EINVAL) {
                ALOGERR("Failed VIDIOC_S_CTRL: framerate=%d", ctrl.value);
                return false;
            }
        }

        ALOGD_TEST("VIDIOC_S_CTRL: framerate=%d", ctrl.value);
        mFramerateChanged = false;
    }

    return prepareExecute(*getLayer(0), SOURCE) && prepareExecute(getCanvas(), TARGET);
}

bool isVotf(AcrylicLayer *layer)
{
    if ((layer->getLayerDataLength() == sizeof(unsigned int [4])) &&
        (reinterpret_cast<unsigned int *>(layer->getLayerData())[0] == 1))
        return true;

    return false;
}

bool AcrylicCompositorMSCL9810::queueBuffer(AcrylicCanvas &canvas, v4l2_buf_type buftype, int *fence, bool needReleaseFence)
{
    bool output = V4L2_TYPE_IS_OUTPUT(buftype);
    bool dmabuf = canvas.getBufferType() == AcrylicCanvas::MT_DMABUF;
    v4l2_buffer buffer;
    v4l2_plane plane[4];

    memset(&buffer, 0, sizeof(buffer));

    buffer.type = buftype;
    buffer.memory = dmabuf ? V4L2_MEMORY_DMABUF : V4L2_MEMORY_USERPTR;
    if (canvas.getFence() >= 0) {
        buffer.flags = mUseFenceFlag;
        buffer.reserved = canvas.getFence();
    } else if (needReleaseFence) {
        /*
         * In this case, release fence is requested without acquire fence.
         * For compatibility, flags should be set as below.
         * If (cap.device_caps & V4L2_CAPFENCES) is true, flags should be V4L2_BUF_FLAG_OUT_FENCE only.
         * else, flags should be V4L2_BUF_FLAG_USE_SYNC and reserved should be -1.
         */
        buffer.flags = mUseFenceFlag & ~V4L2_BUF_FLAG_IN_FENCE;
        buffer.reserved = -1;
    }

    for (unsigned int i = 0; i < canvas.getBufferCount(); i++) {
        plane[i].length = canvas.getBufferLength(i);
        if (output)
            plane[i].bytesused = halfmt_plane_length(canvas.getFormat(), i,
                                    canvas.getImageDimension().hori, mCurrentCrop[SOURCE].size.vert);

        if (dmabuf)
            plane[i].m.fd = canvas.getDmabuf(i);
        else
            plane[i].m.userptr = reinterpret_cast<unsigned long>(canvas.getUserptr(i));

        plane[i].data_offset = canvas.getOffset(i);

        ALOGD_TEST("VIDIOC_QBUF: plane[%d] .length=%d, .bytesused=%d, .m.fd=%d/userptr=%#lx, .offset=%#x",
                   i, plane[i].length, plane[i].bytesused,
                   plane[i].m.fd, plane[i].m.userptr, plane[i].data_offset);
    }

    if (!output) {
        if (mVotfSupported) {
            enum {
                VOTF_REQUESTED,
                VOTF_DPU_DMA_IDX,
                VOTF_TRS_IDX,
                VOTF_BUF_IDX,
            };

            if (isVotf(getLayer(0))) {
                unsigned int *votfData = reinterpret_cast<unsigned int *>(getLayer(0)->getLayerData());

                plane[0].reserved[VOTF_REQUESTED] = 1;
                plane[0].reserved[VOTF_DPU_DMA_IDX] = votfData[1];
                plane[0].reserved[VOTF_TRS_IDX] = votfData[2];
                plane[0].reserved[VOTF_BUF_IDX] = votfData[3];
            } else {
                plane[0].reserved[VOTF_REQUESTED] = 0;
            }
        } else if (isVotf(getLayer(0))) {
            ALOGERR("vOTF is not supported, but vOTF is requested");
            return false;
        }
    }

    buffer.length = canvas.getBufferCount();
    buffer.m.planes = plane;

    ALOGD_TEST("             .type=%d, .memory=%d, .flags=%d, .length=%d, .reserved=%d, .reserved2=%d",
               buffer.type, buffer.memory, buffer.flags, buffer.length, buffer.reserved, buffer.reserved2);

    if (mDev.ioctl(VIDIOC_QBUF, &buffer) < 0) {
        canvas.setFence(-1);
        ALOGERR("Failed VIDOC_QBUF: type=%d, memory=%d", buffer.type, buffer.memory);
        return false;
    }

    // NOTE: V4L2 clears V4L2_BUF_FLAG_USE_SYNC on return.
    if (canvas.getFence() >= 0 || needReleaseFence) {
        if (fence)
            *fence = buffer.reserved;
        else
            close(buffer.reserved); // no one waits for the release fence
    }

    canvas.setFence(-1);

    return true;
}

bool AcrylicCompositorMSCL9810::queueBuffer(int fence[], unsigned int num_fences)
{
    int release_fence[NUM_IMAGES] = {-1, -1};
    int fence_count = num_fences;
    unsigned int i = 0;
    bool success;

    success = queueBuffer(*getLayer(0), mCurrentTypeBuf[SOURCE], &release_fence[SOURCE], (fence_count > 0));
    if (success) {
        if (!queueBuffer(getCanvas(), mCurrentTypeBuf[TARGET], &release_fence[TARGET], (--fence_count > 0))) {
            // reset the state of the output path.
            // ignore even though resetMode() is failed. Nothing to do any more.
            resetMode();
            if (release_fence[SOURCE] >= 0)
                close(release_fence[SOURCE]);
            success = false;
        }
    }

    if (success) {
        setDeviceState(SOURCE, STATE_QBUF);
        setDeviceState(TARGET, STATE_QBUF);

        unsigned int max_fences = num_fences < NUM_IMAGES ? num_fences : NUM_IMAGES;

        for ( ; i < max_fences; i++)
            fence[i] = release_fence[i];

        // release the unwanted release fences if num_fences < 2
        for (unsigned int j = i; j < 2; j++)
            if (release_fence[j] >= 0)
                close(release_fence[j]);
    }

    // set -1 to unset fence entries if num_fences > 2
    for ( ; i < num_fences; i++)
        fence[i] = -1;

    getCanvas().clearSettingModified();
    getLayer(0)->clearSettingModified();

    getCanvas().setFence(-1);
    getLayer(0)->setFence(-1);

    return success;
}

#define DATASPACE_RANGE_FULL        1
#define DATASPACE_RANGE_LIMITED     0
bool AcrylicCompositorMSCL9810::configureCSC()
{
    bool csc_req = false;
    AcrylicCanvas *cscCanvas;

    if ((halfmt_chroma_subsampling(getLayer(0)->getFormat()) == 0x11) &&
        (halfmt_chroma_subsampling(getCanvas().getFormat()) != 0x11)) {
        // RGB of sRGB -> Y'CbCr
        csc_req = true;
        cscCanvas = &getCanvas();
    } else if ((halfmt_chroma_subsampling(getLayer(0)->getFormat()) != 0x11) &&
        (halfmt_chroma_subsampling(getCanvas().getFormat()) == 0x11)) {
        // Y'CbCr -> RGB of sRGB
        csc_req = true;
        cscCanvas = getLayer(0);
    }

    if (csc_req) {
        uint32_t cscSel = 0;
        uint32_t cscRange = 0;

        hw2d_coord_t coord = cscCanvas->getImageDimension();

        cscSel = haldataspace_to_v4l2(cscCanvas->getDataspace(), coord.hori, coord.vert);
        switch (cscSel) {
            case V4L2_COLORSPACE_SRGB:
            case V4L2_COLORSPACE_REC709:
                cscSel = V4L2_COLORSPACE_REC709;
                break;
            case V4L2_COLORSPACE_JPEG:
            case V4L2_COLORSPACE_SMPTE170M:
                cscSel = V4L2_COLORSPACE_SMPTE170M;
                break;
            case V4L2_COLORSPACE_BT2020:
                cscSel = V4L2_COLORSPACE_BT2020;
                break;
        }

        cscRange = haldataspace_to_range(cscCanvas->getDataspace(), coord.hori, coord.vert);

        v4l2_control ctrl;

        ctrl.id = V4L2_CID_CSC_EQ;
        ctrl.value = cscSel;
        ALOGD_TEST("VIDIOC_S_CTRL: csc_matrix_sel=%d", ctrl.value);
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure csc matrix to %d", ctrl.value);
            return false;
        }

        ctrl.id = V4L2_CID_CSC_RANGE;
        ctrl.value = cscRange;
        ALOGD_TEST("VIDIOC_S_CTRL: csc_range=%d", ctrl.value);
        if (mDev.ioctl(VIDIOC_S_CTRL, &ctrl) < 0) {
            ALOGERR("Failed to configure csc range to %d", ctrl.value);
            return false;
        }
    }

    return true;
}

bool AcrylicCompositorMSCL9810::execute(int fence[], unsigned int num_fences)
{
    if (!validateAllLayers())
        return false;

    LOGASSERT(layerCount() == 1, "Number of layer is not 1 but %d", layerCount());

    if (!waitExecution(0)) {
        ALOGE("Error occurred in the previous image processing");
        return false;
    }

    if (!setTransform())
        return false;

    if (!resetMode())
        return false;

    if (!changeMode(*getLayer(0), SOURCE) || !changeMode(getCanvas(), TARGET))
        return false;

    if (!prepareExecute())
        return false;

    return queueBuffer(fence, num_fences);
}

bool AcrylicCompositorMSCL9810::execute(int *handle)
{
    bool success = execute(NULL, 0);

    if (success) {
        if (handle != NULL)
            *handle = 1; /* dummy handle */
        else
            success = waitExecution(0);
    }

    return success;
}

bool AcrylicCompositorMSCL9810::dequeueBuffer()
{
    LOGASSERT(testDeviceState(SOURCE, STATE_QBUF) == testDeviceState(TARGET, STATE_QBUF),
              "State of the device is different: source %#x, target %#x",
              mDeviceState[SOURCE], mDeviceState[TARGET]);

    v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));

    if (!dequeueBuffer(SOURCE, &buffer))
        return false;

    if (!dequeueBuffer(TARGET, &buffer))
        return false;

    return true;
}

bool AcrylicCompositorMSCL9810::dequeueBuffer(BUFDIRECTION dir, v4l2_buffer *buffer)
{
    if (testDeviceState(dir, STATE_QBUF)) {
        buffer->type = mCurrentTypeBuf[dir];
        buffer->memory = (mCurrentTypeMem[dir] == AcrylicCanvas::MT_DMABUF) ? V4L2_MEMORY_DMABUF
                                                                           : V4L2_MEMORY_USERPTR;

        v4l2_plane planes[4];

        memset(planes, 0, sizeof(planes));

        buffer->length = 4;
        buffer->m.planes = planes;

        ALOGD_TEST("VIDIOC_DQBUF: v4l2_buffer .type=%d, .memory=%d, .length=%d",
                   buffer->type, buffer->memory, buffer->length);

        if (mDev.ioctl(VIDIOC_DQBUF, buffer) < 0) {
            ALOGERR("Failed VIDIOC_DQBUF: type=%d, memory=%d", buffer->type, buffer->memory);
            return false;
        } else if (!!(buffer->flags & V4L2_BUF_FLAG_ERROR)) {
            ALOGI("Error during streaming: type=%d, memory=%d", buffer->type, buffer->memory);
        }

        clearDeviceState(dir, STATE_QBUF);

        // The clients of V4L2 capture/m2m device should identify and verify the payload
        // written by the device and the expected payload but MSCL driver does not specify
        // the payload written by it.
        // The driver assumes that the payload of the result of MSCL is always the same as
        // the expected and it is always true in fact.
        // Therefore checking the payload written by the device is ignored here.
    }

    return true;

}

bool AcrylicCompositorMSCL9810::waitExecution(int __unused handle)
{
    return dequeueBuffer();
}

bool AcrylicCompositorMSCL9810::requestPerformanceQoS(AcrylicPerformanceRequest *request)
{
    uint32_t framerate;

    if (!request || (request->getFrameCount() == 0))
        framerate = 0;
    else
        framerate = request->getFrame(0)->mFrameRate;

    if (mFramerate != framerate) {
        mFramerate = framerate;
        mFramerateChanged = true;
    }

    return true;
}
