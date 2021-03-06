#pragma once

#include "software_global.h"

#include "SoftwareRenderer.h"

#include <string>
#include <vector>

namespace paysages {
namespace software {

/**
 * Software rendering inside a Canvas surface.
 *
 * This class launches the rasterization process into canvas portions and
 * redirects post processing to the software renderer.
 *
 * It tries to keep a canvas portion rasterized ahead of the post processing.
 */
class SOFTWARESHARED_EXPORT SoftwareCanvasRenderer : public SoftwareRenderer {
  public:
    SoftwareCanvasRenderer(Scenery *scenery);
    virtual ~SoftwareCanvasRenderer();

    inline const Canvas *getCanvas() const {
        return canvas;
    }
    inline RenderProgress *getProgressHelper() const {
        return progress;
    }
    inline bool isFinished() const {
        return finished;
    }

    inline Rasterizer *getSkyRasterizer() const {
        return rasterizer_sky;
    }
    inline Rasterizer *getWaterRasterizer() const {
        return rasterizer_water;
    }
    inline Rasterizer *getTerrainRasterizer() const {
        return rasterizer_terrain;
    }
    inline Rasterizer *getVegetationRasterizer() const {
        return rasterizer_vegetation;
    }

    virtual void setQuality(double factor) override;

    /**
     * Clear the rasterizers list, and put a single one.
     */
    void setSoloRasterizer(Rasterizer *rasterizer);

    /**
     * Get the global rendering progress (0.0-1.0).
     */
    double getProgress() const;

    /**
     * Set the renderer configuration.
     */
    void setConfig(const RenderConfig &config);

    /**
     * Enable or disable the post processing.
     */
    void enablePostprocess(bool enabled);

    /**
     * Set the rendering size in pixels.
     *
     * Set 'samples' to something bigger than 1 to allow for the multi-sampling of pixels.
     */
    void setSize(int width, int height, int samples = 1);

    /**
     * Start the two-pass render process.
     */
    void render();

    /**
     * Interrupt the render process.
     */
    void interrupt();

    /**
     * Get a rasterizer by its client id.
     */
    const Rasterizer &getRasterizer(int client_id) const;

    /**
     * Save the rendered canvas to a picture file on disk.
     *
     * Returns true if the save was successful.
     */
    bool saveToDisk(const string &filepath) const;

  protected:
    /**
     * Rasterize the scenery into a canvas portion.
     */
    void rasterize(CanvasPortion *portion);

    /**
     * Apply pixel shader to fragments stored in the CanvasPortion.
     */
    void applyPixelShader(CanvasPortion *portion);

  private:
    RenderProgress *progress;

    Canvas *canvas;
    int samples;

    vector<Rasterizer *> rasterizers;
    Rasterizer *rasterizer_sky;
    Rasterizer *rasterizer_water;
    Rasterizer *rasterizer_terrain;
    Rasterizer *rasterizer_vegetation;

    bool started;
    bool finished;
    bool interrupted;

    bool postprocess_enabled;

    ParallelWork *current_work;
};
}
}
