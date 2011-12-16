#include <math.h>

#include "shared/types.h"
#include "shared/functions.h"
#include "shared/constants.h"
#include "shared/globals.h"
#include "clouds.h"

#define MAX_LAYERS 10

static int _layers_count = 0;
static CloudsDefinition _layers[MAX_LAYERS];
static CloudsQuality _quality;
static CloudsEnvironment _environment;

typedef struct
{
    Vector3 start;
    Vector3 end;
    double length;
} CloudSegment;

void cloudsInit()
{
    _layers_count = 0;
    
    _quality.precision = 0.5;
}

void cloudsSave(FILE* f)
{
    int i;
    CloudsDefinition* layer;
    
    toolsSaveInt(f, _layers_count);
    for (i = 0; i < _layers_count; i++)
    {
        layer = _layers + i;
        toolsSaveDouble(f, layer->ycenter);
        toolsSaveDouble(f, layer->ymin);
        toolsSaveDouble(f, layer->ymax);
        noiseSave(layer->noise, f);
        colorSave(layer->color, f);
        toolsSaveDouble(f, layer->scaling);
        toolsSaveDouble(f, layer->coverage);
    }
}

void cloudsLoad(FILE* f)
{
    int i;
    CloudsDefinition* layer;
    
    _layers_count = toolsLoadInt(f);
    for (i = 0; i < _layers_count; i++)
    {
        layer = _layers + i;
        layer->ycenter = toolsLoadDouble(f);
        layer->ymin = toolsLoadDouble(f);
        layer->ymax = toolsLoadDouble(f);
        noiseLoad(layer->noise, f);
        layer->color = colorLoad(f);
        layer->scaling = toolsLoadDouble(f);
        layer->coverage = toolsLoadDouble(f);
    }
}

int cloudsGetLayerCount()
{
    return _layers_count;
}

int cloudsAddLayer()
{
    CloudsDefinition* layer;
    
    layer = _layers + _layers_count;
    layer->noise = noiseCreateGenerator();
    layer->coverage = 0.0;
    
    return _layers_count++;
}

void cloudsDeleteLayer(int layer)
{
    /* TODO */
}

CloudsDefinition cloudsCreateDefinition()
{
    CloudsDefinition result;
    
    result.color = COLOR_WHITE;
    result.coverage = 0.0;
    result.noise = noiseCreateGenerator();
    result.scaling = 1.0;
    result.ymin = 0.0;
    result.ycenter = 0.0;
    result.ymax = 0.0;
    
    return result;
}

void cloudsDeleteDefinition(CloudsDefinition definition)
{
    noiseDeleteGenerator(definition.noise);
}

void cloudsCopyDefinition(CloudsDefinition source, CloudsDefinition* destination)
{
    NoiseGenerator* noise;
    
    noise = destination->noise;
    *destination = source;
    destination->noise = noise;
    noiseCopy(source.noise, destination->noise);
}

void cloudsSetDefinition(int layer, CloudsDefinition definition)
{
    CloudsDefinition* destination;
    
    if (layer >= 0 && layer < _layers_count)
    {
        destination = _layers + layer;
        cloudsCopyDefinition(definition, destination);
        if (destination->coverage < 0.5)
        {
            noiseNormalizeHeight(destination->noise, -1.0, destination->coverage * 2.0, 0);
        }
        else
        {
            noiseNormalizeHeight(destination->noise, -(1.0 - destination->coverage) * 2.0, 1.0, 0);
        }
    }
}

CloudsDefinition cloudsGetDefinition(int layer)
{
    return _layers[layer];
}

void cloudsSetQuality(CloudsQuality quality)
{
    _quality = quality;
}

CloudsQuality cloudsGetQuality()
{
    return _quality;
}

/*int cloudsAddLayer(double ymin, double ycenter, double ymax, Color color, double scaling, double coverage)
{
    CloudsDefinition* layer = _layers + _layers_count;

    layer->ycenter = ycenter;
    layer->ymin = ymin;
    layer->ymax = ymax;
    layer->color = color;
    layer->scaling = scaling;
    layer->coverage = coverage;

    layer->noise = noiseCreateGenerator();
    noiseGenerateBaseNoise(layer->noise, 262144);
    noiseAddLevelSimple(layer->noise, scaling, 0.3);
    noiseAddLevelSimple(layer->noise, scaling / 2.0, 0.2);
    noiseAddLevelSimple(layer->noise, scaling / 4.0, 0.1);
    noiseAddLevelSimple(layer->noise, scaling / 10.0, 0.05);
    noiseAddLevelSimple(layer->noise, scaling / 20.0, 0.03);
    noiseAddLevelSimple(layer->noise, scaling / 40.0, 0.02);
    noiseAddLevelSimple(layer->noise, scaling / 60.0, 0.01);
    noiseAddLevelSimple(layer->noise, scaling / 80.0, 0.005);
    noiseAddLevelSimple(layer->noise, scaling / 100.0, 0.02);
    noiseAddLevelSimple(layer->noise, scaling / 150.0, 0.005);
    noiseAddLevelSimple(layer->noise, scaling / 200.0, 0.003);
    noiseAddLevelSimple(layer->noise, scaling / 400.0, 0.008);
    noiseAddLevelSimple(layer->noise, scaling / 800.0, 0.001);
    noiseAddLevelSimple(layer->noise, scaling / 1000.0, 0.0005);
    if (coverage < 0.5)
    {
        noiseNormalizeHeight(layer->noise, -1.0, coverage * 2.0, 0);
    }
    else
    {
        noiseNormalizeHeight(layer->noise, -(1.0 - coverage) * 2.0, 1.0, 0);
    }

    return _layers_count++;
}*/

static inline double _getDistanceToBorder(CloudsDefinition* layer, Vector3 position, double detail)
{
    double val, min;

    if (position.y > layer->ycenter)
    {
        min = (position.y - layer->ycenter) / (layer->ymax - layer->ycenter);
    }
    else
    {
        min = (layer->ycenter - position.y) / (layer->ycenter - layer->ymin);
    }

    val = noiseGet3DDetail(layer->noise, position.x, position.y, position.z, detail);

    return (val - min) * layer->scaling;
}

static inline Vector3 _getNormal(CloudsDefinition* layer, Vector3 position, double detail)
{
    Vector3 result = {0.0, 0.0, 0.0};
    double val, dval;

    val = noiseGet3DDetail(layer->noise, position.x, position.y, position.z, detail);

    dval = val - noiseGet3DDetail(layer->noise, position.x + detail, position.y, position.z, detail);
    result.x += dval;

    dval = val - noiseGet3DDetail(layer->noise, position.x - detail, position.y, position.z, detail);
    result.x -= dval;

    dval = val - noiseGet3DDetail(layer->noise, position.x, position.y + detail, position.z, detail);
    result.y += dval;

    dval = val - noiseGet3DDetail(layer->noise, position.x, position.y - detail, position.z, detail);
    result.y -= dval;

    dval = val - noiseGet3DDetail(layer->noise, position.x, position.y, position.z + detail, detail);
    result.z += dval;

    dval = val - noiseGet3DDetail(layer->noise, position.x, position.y, position.z - detail, detail);
    result.z -= dval;

    return result;
}

/**
 * Optimize the search limits in a layer.
 * 
 * @param layer The cloud layer
 * @param start Start of the search to optimize
 * @param end End of the search to optimize
 * @return 0 if the search is useless
 */
static int _optimizeSearchLimits(CloudsDefinition* layer, Vector3* start, Vector3* end)
{
    Vector3 diff;
    
    if (start->y > layer->ymax)
    {
        if (end->y >= layer->ymax)
        {
            return 0;
        }
        else
        {
            diff = v3Sub(*end, *start);
            *start = v3Add(*start, v3Scale(diff, (layer->ymax - start->y) / diff.y));
            if (end->y < layer->ymin)
            {
                *end = v3Add(*end, v3Scale(diff, (layer->ymin - end->y) / diff.y));
            }
        }
    }
    else if (start->y < layer->ymin)
    {
        if (end->y <= layer->ymin)
        {
            return 0;
        }
        else
        {
            diff = v3Sub(*end, *start);
            *start = v3Add(*start, v3Scale(diff, (layer->ymin - start->y) / diff.y));
            if (end->y > layer->ymax)
            {
                *end = v3Add(*end, v3Scale(diff, (layer->ymax - end->y) / diff.y));
            }
        }
    }
    else /* start is inside layer */
    {
        diff = v3Sub(*end, *start);
        if (end->y > layer->ymax)
        {
            *end = v3Add(*start, v3Scale(diff, (layer->ymax - start->y) / diff.y));
        }
        else if (end->y < layer->ymin)
        {
            *end = v3Add(*start, v3Scale(diff, (layer->ymin - start->y) / diff.y));
        }
    }
    
    /* TODO Limit the search length */
    return 1;
}

/**
 * Go through the cloud layer to find segments (parts of the lookup that are inside the cloud).
 * 
 * @param definition The cloud layer
 * @param quality Render quality
 * @param start Start position of the lookup (already optimized)
 * @param direction Normalized direction of the lookup
 * @param detail Level of noise detail required
 * @param max_segments Maximum number of segments to collect
 * @param max_inside_length Maximum length to spend inside the cloud
 * @param max_total_length Maximum lookup length
 * @param inside_length Resulting length inside cloud (sum of all segments length)
 * @param total_length Resulting lookup length
 * @param out_segments Allocated space to fill found segments
 * @return Number of segments found
 */
static int _findSegments(CloudsDefinition* definition, CloudsQuality* quality, Vector3 start, Vector3 direction, double detail, int max_segments, double max_inside_length, double max_total_length, double* inside_length, double* total_length, CloudSegment* out_segments)
{
    int inside, segment_count;
    double current_total_length, current_inside_length;
    double step_length, segment_length, remaining_length;
    double noise_distance, last_noise_distance;
    Vector3 walker, step, segment_start;
    
    if (max_segments <= 0)
    {
        return 0;
    }

    segment_count = 0;
    current_total_length = 0.0;
    current_inside_length = 0.0;
    segment_length = 0.0;
    walker = start;
    noise_distance = _getDistanceToBorder(definition, start, detail) * quality->precision;
    inside = (noise_distance > 0.0) ? 1 : 0;
    step = v3Scale(direction, quality->precision);

    do
    {
        walker = v3Add(walker, step);
        step_length = v3Norm(step);
        last_noise_distance = noise_distance;
        noise_distance = _getDistanceToBorder(definition, walker, detail) * quality->precision;
        current_total_length += step_length;
        
        if (noise_distance > 0.0)
        {
            if (inside)
            {
                // inside the cloud
                segment_length += step_length;
                current_inside_length += step_length;
                step = v3Scale(direction, (noise_distance < quality->precision) ? quality->precision : noise_distance);
            }
            else
            {
                // entering the cloud
                inside = 1;
                segment_length = step_length * noise_distance / (noise_distance - last_noise_distance);
                segment_start = v3Add(walker, v3Scale(direction, -segment_length));
                current_inside_length += segment_length;
                step = v3Scale(direction, quality->precision);
            }
        }
        else
        {
            if (inside)
            {
                // exiting the cloud
                remaining_length = step_length * last_noise_distance / (last_noise_distance - noise_distance);
                segment_length += remaining_length;
                current_inside_length += remaining_length;
                
                out_segments->start = segment_start;
                out_segments->end = v3Add(walker, v3Scale(direction, remaining_length - step_length));
                out_segments->length = segment_length;
                out_segments++;
                if (++segment_count >= max_segments)
                {
                    break;
                }
                
                inside = 0;
                step = v3Scale(direction, quality->precision);
            }
            else
            {
                // searching for a cloud
                step = v3Scale(direction, (noise_distance > -quality->precision) ? quality->precision : -noise_distance);
            }
        }
    } while (inside || (walker.y <= definition->ymax + 0.001 && walker.y >= definition->ymin - 0.001 && current_total_length < max_total_length && current_inside_length < max_inside_length));
    
    *total_length = current_total_length;
    *inside_length = current_inside_length;
    return segment_count;
}

static Color _applyLayerLighting(CloudsDefinition* definition, CloudsQuality* quality, Vector3 position, Color base, double detail)
{
    Vector3 direction, normal;
    double inside_depth, total_depth;
    CloudSegment segments[20];
    Color result;

    normal = _getNormal(definition, position, 0.5);
    normal = v3Add(normal, _getNormal(definition, position, 0.2));
    normal = v3Add(normal, _getNormal(definition, position, 0.1));
    result = lightingApply(position, normal, 0.0, base, 0.3, 0.1);
    
    direction = sun_direction_inv;
    detail = (detail < 0.1) ? 0.1 : detail;
    /* FIXME Dont hard-code max_total_length */
    _findSegments(definition, quality, position, direction, detail, 20, 50.0, 300.0, &inside_depth, &total_depth, segments);

    inside_depth *= 0.02;
    if (inside_depth > 1.0)
    {
        inside_depth = 1.0;
    }
    
    result.r = base.r * sun_color_lum * (0.9 - 0.2 * inside_depth) + result.r * (0.1 + 0.1 * inside_depth) + (0.1 - inside_depth * 0.1) * sun_color_lum;
    result.g = base.g * sun_color_lum * (0.9 - 0.2 * inside_depth) + result.g * (0.1 + 0.1 * inside_depth) + (0.1 - inside_depth * 0.1) * sun_color_lum;
    result.b = base.b * sun_color_lum * (0.9 - 0.2 * inside_depth) + result.b * (0.1 + 0.1 * inside_depth) + (0.1 - inside_depth * 0.1) * sun_color_lum;

    return result;
}

Color cloudsGetColorCustom(Vector3 start, Vector3 end, CloudsDefinition* definition, CloudsQuality* quality, CloudsEnvironment* environment)
{
    int i, segment_count;
    double max_length, detail, total_length, inside_length;
    Vector3 direction;
    Color result, col;
    CloudSegment segments[20];

    if (quality == NULL)
    {
        quality = &_quality;
    }
    if (environment == NULL)
    {
        environment = &_environment;
    }
    
    if (!_optimizeSearchLimits(definition, &start, &end))
    {
        return COLOR_TRANSPARENT;
    }
    
    direction = v3Sub(end, start);
    max_length = v3Norm(direction);
    direction = v3Normalize(direction);
    result = COLOR_TRANSPARENT;

    /* TODO Flexible precision */
    detail = renderGetPrecision(start) / definition->scaling;
    
    segment_count = _findSegments(definition, quality, start, direction, detail, 20, 60.0, max_length, &inside_length, &total_length, segments);
    for (i = 0; i < segment_count; i++)
    {
        col = _applyLayerLighting(definition, quality, segments[i].start, definition->color, detail);
        col.a = (segments[i].length >= 50.0) ? 1.0 : (segments[i].length / 50.0);
        colorMask(&result, &col);
    }
    return result;
}

Color cloudsGetColor(Vector3 start, Vector3 end)
{
    int i;
    Color layer_color, result;

    if (end.y < start.y)
    {
        return cloudsGetColor(end, start);
    }

    if (_layers_count < 1 || end.y - start.y < 0.001)
    {
        return COLOR_TRANSPARENT;
    }

    result = COLOR_TRANSPARENT;
    for (i = 0; i < _layers_count; i++)
    {
        layer_color = cloudsGetColorCustom(start, end, _layers + i, &_quality, &_environment);
        if (layer_color.a > 0.0)
        {
            colorMask(&result, &layer_color);
        }
    }

    return result;
}
