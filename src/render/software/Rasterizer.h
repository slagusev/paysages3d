#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "software_global.h"

namespace paysages {
namespace software {

typedef struct ScanPoint ScanPoint;
typedef struct RenderScanlines RenderScanlines;

/**
 * @brief Base abstract class for scenery pieces that can be rasterized to polygons.
 */
class SOFTWARESHARED_EXPORT Rasterizer
{
public:
    Rasterizer(SoftwareRenderer *renderer, int client_id, const Color &color);
    virtual ~Rasterizer();

    inline SoftwareRenderer *getRenderer() const {return renderer;}

    virtual void rasterizeToCanvas(CanvasPortion* canvas) = 0;
    virtual Color shadeFragment(const CanvasFragment &fragment) const = 0;
    virtual void interrupt();

protected:
    void addPredictedPolys(int count=1);
    void addDonePolys(int count=1);

    void pushProjectedTriangle(CanvasPortion *canvas, const Vector3 &pixel1, const Vector3 &pixel2, const Vector3 &pixel3, const Vector3 &location1, const Vector3 &location2, const Vector3 &location3);

    void pushTriangle(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3);
    void pushQuad(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &v4);
    void pushDisplacedTriangle(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &ov1, const Vector3 &ov2, const Vector3 &ov3);
    void pushDisplacedQuad(CanvasPortion *canvas, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, const Vector3 &v4, const Vector3 &ov1, const Vector3 &ov2, const Vector3 &ov3, const Vector3 &ov4);

    Color* color;
    SoftwareRenderer *renderer;
    int client_id;
    bool interrupted;

private:
    void scanGetDiff(ScanPoint *v1, ScanPoint *v2, ScanPoint *result);
    void scanInterpolate(CameraDefinition *camera, ScanPoint *v1, ScanPoint *diff, double value, ScanPoint *result);
    void pushScanPoint(CanvasPortion *canvas, RenderScanlines *scanlines, ScanPoint *point);
    void pushScanLineEdge(CanvasPortion *canvas, RenderScanlines *scanlines, ScanPoint *point1, ScanPoint *point2);
    void renderScanLines(CanvasPortion *canvas, RenderScanlines *scanlines);

    int predicted_poly_count;
    int done_poly_count;
};

}
}

#endif // RASTERIZER_H