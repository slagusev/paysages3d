#include "SoftwareCanvasRenderer.h"

#include "Rasterizer.h"
#include "SoftwareRenderer.h"
#include "Canvas.h"
#include "TerrainRasterizer.h"
#include "WaterRasterizer.h"
#include "SkyRasterizer.h"
#include "CameraDefinition.h"
#include "ParallelWork.h"
#include "CanvasPortion.h"
#include "CanvasPixelShader.h"

SoftwareCanvasRenderer::SoftwareCanvasRenderer()
{
    started = false;
    canvas = new Canvas();

    rasterizers.push_back(new SkyRasterizer(this, 0));
    rasterizers.push_back(new WaterRasterizer(this, 1));
    rasterizers.push_back(new TerrainRasterizer(this, 2));
}

SoftwareCanvasRenderer::~SoftwareCanvasRenderer()
{
    delete canvas;

    for (auto &rasterizer: rasterizers)
    {
        delete rasterizer;
    }
}

void SoftwareCanvasRenderer::setSize(int width, int height, int samples)
{
    if (not started)
    {
        canvas->setSize(width * samples, height * samples);
    }
}

void SoftwareCanvasRenderer::render()
{
    // TEMP
    started = true;
    CanvasPortion *portion = canvas->at(0, 0);
    render_width = canvas->getWidth();
    render_height = canvas->getHeight();
    render_quality = 3;

    render_camera->setRenderSize(canvas->getWidth(), canvas->getHeight());

    prepare();

    rasterize(portion, true);
    postProcess(portion, true);
}

const std::vector<Rasterizer *> &SoftwareCanvasRenderer::getRasterizers() const
{
    return rasterizers;
}

const Rasterizer &SoftwareCanvasRenderer::getRasterizer(int client_id) const
{
    return *(getRasterizers()[client_id]);
}

void SoftwareCanvasRenderer::rasterize(CanvasPortion *portion, bool threaded)
{
    for (auto &rasterizer:getRasterizers())
    {
        rasterizer->rasterizeToCanvas(portion);
    }
}

void SoftwareCanvasRenderer::postProcess(CanvasPortion *portion, bool threaded)
{
    // Subdivide in chunks
    int chunk_size = 32;
    int chunks_x = (portion->getWidth() - 1) / chunk_size + 1;
    int chunks_y = (portion->getHeight() - 1) / chunk_size + 1;
    int units = chunks_x * chunks_y;

    // Render chunks in parallel
    CanvasPixelShader shader(*this, portion, chunk_size, chunks_x, chunks_y);
    ParallelWork work(&shader, units);
    work.perform();
}
