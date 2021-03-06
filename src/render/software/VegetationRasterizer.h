#pragma once

#include "software_global.h"

#include "TerrainRasterizer.h"

namespace paysages {
namespace software {

class SOFTWARESHARED_EXPORT VegetationRasterizer : public TerrainRasterizer {
  public:
    VegetationRasterizer(SoftwareRenderer *renderer, RenderProgress *progress, unsigned short client_id);

    /**
     * Returns true if the rasterization process is useful.
     */
    bool isUseful() const;

    virtual int prepareRasterization() override;
    virtual void rasterizeToCanvas(CanvasPortion *canvas) override;
    virtual Color shadeFragment(const CanvasFragment &fragment, const CanvasFragment *previous) const override;
};
}
}
