#include "TerrainRasterizer.h"

#include "SoftwareRenderer.h"
#include "BoundingBox.h"
#include "CameraDefinition.h"

#include "tools/parallel.h"
#include "terrain/public.h"
#include "water/public.h"
#include "textures/public.h"

TerrainRasterizer::TerrainRasterizer(SoftwareRenderer* renderer):
    renderer(renderer)
{
}

static inline Vector3 _getPoint(TerrainDefinition*, SoftwareRenderer* renderer, double x, double z)
{
    Vector3 result;

    result.x = x;
    result.y = renderer->terrain->getHeight(renderer, x, z, 1);
    result.z = z;

    return result;
}

static Color _postProcessFragment(Renderer* renderer_, Vector3 point, void*)
{
    double precision;
    SoftwareRenderer* renderer = (SoftwareRenderer*)renderer_;

    point = _getPoint(renderer->terrain->definition, renderer, point.x, point.z);

    precision = renderer->getPrecision(renderer, point);
    return renderer->terrain->getFinalColor(renderer, point, precision);
}

static void _renderQuad(Renderer* renderer, double x, double z, double size, double water_height)
{
    Vector3 ov1, ov2, ov3, ov4;
    Vector3 dv1, dv2, dv3, dv4;

    ov1.y = ov2.y = ov3.y = ov4.y = 0.0;

    ov1.x = x;
    ov1.z = z;
    dv1 = renderer->terrain->getResult(renderer, x, z, 1, 1).location;

    ov2.x = x;
    ov2.z = z + size;
    dv2 = renderer->terrain->getResult(renderer, x, z + size, 1, 1).location;

    ov3.x = x + size;
    ov3.z = z + size;
    dv3 = renderer->terrain->getResult(renderer, x + size, z + size, 1, 1).location;

    ov4.x = x + size;
    ov4.z = z;
    dv4 = renderer->terrain->getResult(renderer, x + size, z, 1, 1).location;

    if (dv1.y > water_height || dv2.y > water_height || dv3.y > water_height || dv4.y > water_height)
    {
        renderer->pushDisplacedQuad(renderer, dv1, dv2, dv3, dv4, ov1, ov2, ov3, ov4, _postProcessFragment, NULL);
    }
}

void TerrainRasterizer::tessellateChunk(TerrainChunkInfo* chunk, int detail)
{
    if (detail < 1)
    {
        return;
    }

    double water_height = renderer->water->getHeightInfo(renderer).min_height;

    double startx = chunk->point_nw.x;
    double startz = chunk->point_nw.z;
    double size = (chunk->point_ne.x - chunk->point_nw.x) / (double)detail;
    int i, j;

    for (i = 0; i < detail; i++)
    {
        for (j = 0; j < detail; j++)
        {
            _renderQuad(renderer, startx + (double)i * size, startz + (double)j * size, size, water_height);
        }
    }
}

static void _getChunk(Renderer* renderer, TerrainRasterizer::TerrainChunkInfo* chunk, double x, double z, double size, int displaced)
{
    chunk->point_nw = renderer->terrain->getResult(renderer, x, z, 1, displaced).location;
    chunk->point_sw = renderer->terrain->getResult(renderer, x, z + size, 1, displaced).location;
    chunk->point_se = renderer->terrain->getResult(renderer, x + size, z + size, 1, displaced).location;
    chunk->point_ne = renderer->terrain->getResult(renderer, x + size, z, 1, displaced).location;

    double displacement_power;
    if (displaced)
    {
        displacement_power = 0.0;
    }
    else
    {
        displacement_power = texturesGetMaximalDisplacement(renderer->textures->definition);
    }

    BoundingBox box;
    if (displacement_power > 0.0)
    {
        box.pushPoint(v3Add(chunk->point_nw, v3(-displacement_power, displacement_power, -displacement_power)));
        box.pushPoint(v3Add(chunk->point_nw, v3(-displacement_power, -displacement_power, -displacement_power)));
        box.pushPoint(v3Add(chunk->point_sw, v3(-displacement_power, displacement_power, displacement_power)));
        box.pushPoint(v3Add(chunk->point_sw, v3(-displacement_power, -displacement_power, displacement_power)));
        box.pushPoint(v3Add(chunk->point_se, v3(displacement_power, displacement_power, displacement_power)));
        box.pushPoint(v3Add(chunk->point_se, v3(displacement_power, -displacement_power, displacement_power)));
        box.pushPoint(v3Add(chunk->point_ne, v3(displacement_power, displacement_power, -displacement_power)));
        box.pushPoint(v3Add(chunk->point_ne, v3(displacement_power, -displacement_power, -displacement_power)));
    }
    else
    {
        box.pushPoint(chunk->point_nw);
        box.pushPoint(chunk->point_sw);
        box.pushPoint(chunk->point_se);
        box.pushPoint(chunk->point_ne);
    }

    int coverage = renderer->render_camera->isUnprojectedBoxInView(box);
    if (coverage > 0)
    {
        chunk->detail_hint = (int)ceil(sqrt((double)coverage) / (double)(25 - 2 * renderer->render_quality));
        if (chunk->detail_hint > 5 * renderer->render_quality)
        {
            chunk->detail_hint = 5 * renderer->render_quality;
        }
    }
    else
    {
        /* Not in view */
        chunk->detail_hint = -1;
    }
}

void TerrainRasterizer::getTessellationInfo(int displaced)
{
    TerrainChunkInfo chunk;
    int chunk_factor, chunk_count, i;
    Vector3 cam = renderer->getCameraLocation(renderer, VECTOR_ZERO);
    double progress;
    double radius_int, radius_ext;
    double base_chunk_size, chunk_size;

    base_chunk_size = 5.0 / (double)renderer->render_quality;

    chunk_factor = 1;
    chunk_count = 2;
    radius_int = 0.0;
    radius_ext = base_chunk_size;
    chunk_size = base_chunk_size;
    progress = 0.0;

    double cx = cam.x - fmod(cam.x, base_chunk_size);
    double cz = cam.z - fmod(cam.x, base_chunk_size);

    while (radius_int < 20000.0)
    {
        progress = radius_int / 20000.0;
        for (i = 0; i < chunk_count - 1; i++)
        {
            _getChunk(renderer, &chunk, cx - radius_ext + chunk_size * i, cz - radius_ext, chunk_size, displaced);
            if (!processChunk(&chunk, progress))
            {
                return;
            }

            _getChunk(renderer, &chunk, cx + radius_int, cz - radius_ext + chunk_size * i, chunk_size, displaced);
            if (!processChunk(&chunk, progress))
            {
                return;
            }

            _getChunk(renderer, &chunk, cx + radius_int - chunk_size * i, cz + radius_int, chunk_size, displaced);
            if (!processChunk(&chunk, progress))
            {
                return;
            }

            _getChunk(renderer, &chunk, cx - radius_ext, cz + radius_int - chunk_size * i, chunk_size, displaced);
            if (!processChunk(&chunk, progress))
            {
                return;
            }
        }

        if (radius_int > 20.0 && chunk_count % 64 == 0 && (double)chunk_factor < radius_int / 20.0)
        {
            chunk_count /= 2;
            chunk_factor *= 2;
        }
        chunk_count += 2;
        chunk_size = base_chunk_size * chunk_factor;
        radius_int = radius_ext;
        radius_ext += chunk_size;
    }
}

typedef struct
{
    TerrainRasterizer* rasterizer;
    TerrainRasterizer::TerrainChunkInfo chunk;
} ParallelRasterInfo;

static int _parallelJobCallback(ParallelQueue*, int, void* data, int stopping)
{
    ParallelRasterInfo* info = (ParallelRasterInfo*)data;

    if (!stopping)
    {
        info->rasterizer->tessellateChunk(&info->chunk, info->chunk.detail_hint);
    }

    free(data);
    return 0;
}

int TerrainRasterizer::processChunk(TerrainChunkInfo* chunk, double progress)
{
    ParallelRasterInfo* info = new ParallelRasterInfo;

    info->rasterizer = this;
    info->chunk = *chunk;

    if (!parallelQueueAddJob((ParallelQueue*)renderer->customData[0], _parallelJobCallback, info))
    {
        delete info;
    }

    renderer->render_progress = 0.05 * progress;
    return !renderer->render_interrupt;
}

void TerrainRasterizer::renderSurface()
{
    ParallelQueue* queue;
    queue = parallelQueueCreate(0);

    /* TODO Do not use custom data, it could already be used by another module */
    renderer->customData[0] = queue;

    renderer->render_progress = 0.0;
    getTessellationInfo(0);
    renderer->render_progress = 0.05;

    parallelQueueWait(queue);
    parallelQueueDelete(queue);
}