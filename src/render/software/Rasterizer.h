#pragma once

#include "software_global.h"

namespace paysages {
namespace software {

const int RASTERIZER_CLIENT_SKY = 0;
const int RASTERIZER_CLIENT_WATER = 1;
const int RASTERIZER_CLIENT_TERRAIN = 2;
const int RASTERIZER_CLIENT_VEGETATION = 3;

typedef struct ScanPoint ScanPoint;
typedef struct RenderScanlines RenderScanlines;

/**
 * Base abstract class for scenery pieces that can be rasterized to polygons.
 */
class SOFTWARESHARED_EXPORT Rasterizer {
  public:
    Rasterizer(SoftwareRenderer *renderer, RenderProgress *progress, unsigned short client_id, const Color &color);
    virtual ~Rasterizer();

    inline SoftwareRenderer *getRenderer() const {
        return renderer;
    }
    inline int getTriangleCount() const {
        return triangle_count;
    }

    /**
     * Set the rasterization quality factor.
     */
    virtual void setQuality(double factor);

    /**
     * Set the edge length under which to stop auto-cutting triangles near the camera.
     */
    void setAutoCutLimit(double limit);

    /**
     * Abstract method to prepare for the rasterization process, and return the estimated progress count.
     */
    virtual int prepareRasterization() = 0;

    /**
     * Abstract method to effectively do the rasterization on a canvas.
     */
    virtual void rasterizeToCanvas(CanvasPortion *canvas) = 0;

    /**
     * Abstract method to render a fragment stored on a canvas, to a color.
     */
    virtual Color shadeFragment(const CanvasFragment &fragment, const CanvasFragment *previous) const = 0;

    /**
     * Ask for an interrupt in rasterization process.
     */
    virtual void interrupt();

    void setColor(const Color &color);
    void setBackFaceCulling(bool cull);
    void setPerspectiveCorrection(bool active);

    /**
     * Reset the internal triangle counter to 0.
     */
    void resetTriangleCount();

    void pushTriangle(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3);
    void pushDisplacedTriangle(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3,
                               const Vector3 &ov1, const Vector3 &ov2, const Vector3 &ov3);

    void pushQuad(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &v4);
    void pushDisplacedQuad(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3,
                           const Vector3 &v4, const Vector3 &ov1, const Vector3 &ov2, const Vector3 &ov3,
                           const Vector3 &ov4);

  protected:
    bool pushProjectedTriangle(CanvasPortion *canvas, const Vector3 &pixel1, const Vector3 &pixel2,
                               const Vector3 &pixel3, const Vector3 &location1, const Vector3 &location2,
                               const Vector3 &location3);

    Color *color;
    SoftwareRenderer *renderer;
    RenderProgress *progress;
    int client_id;
    bool interrupted;

  private:
    void scanGetDiff(ScanPoint *v1, ScanPoint *v2, ScanPoint *result);
    void scanInterpolate(CameraDefinition *camera, ScanPoint *v1, ScanPoint *diff, double value, ScanPoint *result);
    void pushScanPoint(CanvasPortion *canvas, RenderScanlines *scanlines, ScanPoint *point);
    void pushScanLineEdge(CanvasPortion *canvas, RenderScanlines *scanlines, ScanPoint *point1, ScanPoint *point2);
    void renderScanLines(CanvasPortion *canvas, RenderScanlines *scanlines);

    int triangle_count;
    double auto_cut_limit;
    bool backface_culling;
    bool perspective_correction;
};
}
}
