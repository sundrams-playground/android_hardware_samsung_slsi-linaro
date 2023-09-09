#ifndef GEO_TRANS_INTERFACE_H
#define GEO_TRANS_INTERFACE_H
/**
 * 32 data is allocated for fd of native handle
 * The maximum possible value of native_handle_create is 1024
 */
#define MAX_HANDLE_FD   (32)

/**
 * @file geo_trans_interface.hpp
 * @brief geoTrans common API and data structure definition.
 */

namespace hardware {
namespace geoTransClient {

/**
 * @brief struct describes ROI information.
 */
struct Rect {
    int32_t l;
    int32_t t;
    int32_t r;
    int32_t b;
};

/**
 * @brief struct holds buffer information.
 */
struct BufferData {
    BufferData() : y(NULL), uv(NULL), fdY(-1), fdUV(-1), width(0), height(0) {
        fov.l = 0;
        fov.t = 0;
        fov.r = 0;
        fov.b = 0;
    }
    unsigned char* y;
    unsigned char* uv;
    int fdY;
    int fdUV;
    int32_t width;
    int32_t height;
    Rect fov;
};

/**
 * @brief struct describes grid information for GDC.
 */
typedef struct _GDCGrid {
    int32_t gridX[33][33];
    int32_t gridY[33][33];
} GDCGrid;

/**
 * @brief pixel format.
 */
enum {
    PIXEL_FORMAT_Y_GRAY = 0x0,      // Y only with 8b samples
    PIXEL_FORMAT_YUV_420_SP = 0x1  // YUV 4:2:0 planar, with 8b Y samples, followed by interleaved V/U plane with 8b 2x2 subsampled chroma samples
};

/**
 * @brief target model for GDC.
 */
enum {
    TARGET_GDC_HW = 0x0,
    TARGET_GDC_C_MODEL = 0x1
};

  /**
   * @brief
   * Initialize geoTrans10 client.
   * This call attempts to get geoTransService and open Ion client for memory allocation
   * @return[output] status
   */
int init();

  /**
   * @brief
   * Deinitialize geoTrans10 client.
   * This call attempts to close Ion client for memory allocation
   * @return[output] status
   */
int deinit();

  /**
   * @brief
   * Run CSC Scaler.
   * ----------------------------------------------------
   * CSC Specification
   * ----------------------------------------------------
   * Image resolution ragne : Input [16x16] ~ [8192x8192]
   *                          Output [4x4] ~ [8192x8192]
   * Input/output format    : 8b YCbCr 420
   * Scale                  : x1/4 to x8
   * Alignment              : multiple of 2
   * ----------------------------------------------------
   * @param[dst] destination buffer data
   * @param[src] source buffer data
   * @param[format] pixel format (0x0 or 0x1)
   * @return[output] status
   */
int runCSC(BufferData& dst, BufferData& src, int format = PIXEL_FORMAT_YUV_420_SP);

  /**
   * @brief
   * Run GDC Warper with a GDC grid.
   * ----------------------------------------------------
   * GDC Specification
   * ----------------------------------------------------
   * Image resolution ragne : [96x64] ~ [8192x6144]
   * Input/output format    : 8b YCbCr 420
   * Scale                  : x1 only
   * Alignment              : multiple of 16
   * ----------------------------------------------------
   * @param[dst] destination buffer data
   * @param[src] source buffer data
   * @param[grid] 33x33 grid data for (x,y) axis
   * @param[format] pixel format (0x0 or 0x1)
   * @param[target] target model (0x0 or 0x1)
   * @return[output] status
   */
int runGDCGrid(BufferData& dst, BufferData& src, GDCGrid& grid, int format = PIXEL_FORMAT_YUV_420_SP, int target = TARGET_GDC_HW);

  /**
   * @brief
   * Run GDC Warper with an affine matrix.
   * It internally call geoTransRunGDCGrid(), so it has the same specification.
   * @param[dst] destination buffer data
   * @param[src] source buffer data
   * @param[affine] 3x2 affine matrix
   * @param[format] pixel format (0x0 or 0x1)
   * @param[target] target model (0x0 or 0x1)
   * @return[output] status
   */
int runGDCMatrix(BufferData& dst, BufferData& src, short* affine, int format = PIXEL_FORMAT_YUV_420_SP, int target = TARGET_GDC_HW);

  /**
   * @brief
   * Run bicubic SW scaler.
   * Please note that this call is for a backup processing in case when a HW error occurs
   * @param[dst] destination buffer data
   * @param[src] source buffer data
   * @param[format] pixel format (0x0 or 0x1)
   * @return[output] status
   */
int runInterpBicubic(BufferData& dst, BufferData& src, int format = PIXEL_FORMAT_YUV_420_SP);

} // namespace geoTransClient
} // namespace hardware
#endif
