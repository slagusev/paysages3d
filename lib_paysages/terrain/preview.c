#include "public.h"

#include <stdlib.h>
#include "../tools.h"
#include "../tools/lighting.h"
#include "../renderer.h"

/*
 * Terrain previews.
 */

static void _getLightingStatus(Renderer* renderer, LightStatus* status, Vector3 normal, int opaque)
{
    LightDefinition light;

    UNUSED(renderer);
    UNUSED(normal);
    UNUSED(opaque);

    light.color.r = 0.6;
    light.color.g = 0.6;
    light.color.b = 0.6;
    light.direction.x = -1.0;
    light.direction.y = -0.5;
    light.direction.z = 1.0;
    light.direction = v3Normalize(light.direction);
    light.altered = 1;
    light.reflection = 0.0;
    lightingPushLight(status, &light);

    light.color.r = 0.2;
    light.color.g = 0.2;
    light.color.b = 0.2;
    light.direction.x = 1.0;
    light.direction.y = -0.5;
    light.direction.z = -1.0;
    light.direction = v3Normalize(light.direction);
    light.altered = 0;
    light.reflection = 0.0;
    lightingPushLight(status, &light);
}

static Vector3 _getCameraLocation(Renderer* renderer, Vector3 location)
{
    UNUSED(renderer);
    location.x -= 10.0;
    location.y += 15.0;
    location.z += 10.0;
    return location;
}

Renderer* terrainCreatePreviewRenderer()
{
    Renderer* result = rendererCreate();

    result->render_quality = 3;
    result->getCameraLocation = _getCameraLocation;
    result->atmosphere->getLightingStatus = _getLightingStatus;

    return result;
}

Color terrainGetPreviewColor(Renderer* renderer, double x, double z, double detail)
{
    Vector3 point;

    point.x = x;
    point.y = renderer->terrain->getHeight(renderer, x, z, 1);
    point.z = z;

    return renderer->terrain->getFinalColor(renderer, point, detail);
}
