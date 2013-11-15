#include "clo_density.h"

#include "NoiseGenerator.h"
#include "CloudLayerDefinition.h"
#include "Curve.h"

double cloudsGetLayerCoverage(CloudLayerDefinition* layer, Vector3 location)
{
    if (layer->base_coverage <= 0.0)
    {
        return 0.0;
    }
    else
    {
        double coverage = 0.5 + layer->_coverage_noise->get2DTotal(location.x / layer->shape_scaling, location.z / layer->shape_scaling);
        coverage -= (1.0 - layer->base_coverage);

        coverage *= layer->_coverage_by_altitude->getValue((location.y - layer->lower_altitude) / layer->thickness);

        if (coverage < 0.0)
        {
            return 0.0;
        }
        else if (coverage >= 1.0)
        {
            return 1.0;
        }
        else
        {
            return coverage;
        }
    }
}

double cloudsGetLayerDensity(CloudLayerDefinition* layer, Vector3 location, double coverage)
{
    if (coverage <= 0.0)
    {
        return 0.0;
    }
    else if (coverage >= 1.0)
    {
        return 1.0;
    }
    else
    {
        double density = layer->_shape_noise->get3DTotal(location.x / layer->shape_scaling, location.y / layer->shape_scaling, location.z / layer->shape_scaling);
        density -= (0.5 - coverage);
        return (density <= 0.0) ? 0.0 : density;
    }
}

double cloudsGetEdgeDensity(CloudLayerDefinition* layer, Vector3 location, double layer_density)
{
    if (layer_density <= 0.0)
    {
        return 0.0;
    }
    else if (layer_density >= 1.0)
    {
        return 1.0;
    }
    else
    {
        double density = layer->_edge_noise->get3DTotal(location.x / layer->edge_scaling, location.y / layer->edge_scaling, location.z / layer->edge_scaling);
        density -= (0.5 - layer_density);
        return (density <= 0.0) ? 0.0 : density;
    }
}

static double _fakeGetDensity(Renderer*, CloudLayerDefinition*, Vector3)
{
    return 0.0;
}

static double _fakeGetEdgeDensity(Renderer*, CloudLayerDefinition*, Vector3, double)
{
    return 0.0;
}

void cloudsBindFakeDensityToRenderer(CloudsRenderer* renderer)
{
    renderer->getLayerDensity = _fakeGetDensity;
    renderer->getEdgeDensity = _fakeGetEdgeDensity;
}

static double _realGetDensity(Renderer*, CloudLayerDefinition* layer, Vector3 location)
{
    double coverage = cloudsGetLayerCoverage(layer, location);

    return cloudsGetLayerDensity(layer, location, coverage);
}

static double _realGetEdgeDensity(Renderer*, CloudLayerDefinition* layer, Vector3 location, double layer_density)
{
    return cloudsGetEdgeDensity(layer, location, layer_density);
}

void cloudsBindRealDensityToRenderer(CloudsRenderer* renderer)
{
    renderer->getLayerDensity = _realGetDensity;
    renderer->getEdgeDensity = _realGetEdgeDensity;
}
