#ifndef _PAYSAGES_TERRAIN_PUBLIC_H_
#define _PAYSAGES_TERRAIN_PUBLIC_H_

#include "../shared/types.h"
#include "../noise.h"
#include "../lighting.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TERRAIN_PRESET_STANDARD
} TerrainPreset;

typedef struct
{
    double height;
    double scaling;
    double shadow_smoothing;

    double _detail;
    NoiseGenerator* _height_noise;
    double _min_height;
    double _max_height;
} TerrainDefinition;

typedef double (*FuncTerrainGetHeight)(Renderer* renderer, double x, double z);
typedef Color (*FuncTerrainGetFinalColor)(Renderer* renderer, Vector3 location, double precision);

typedef struct
{
    TerrainDefinition* definition;

    FuncGeneralCastRay castRay;
    FuncLightingAlterLight alterLight;
    FuncTerrainGetHeight getHeight;
    FuncTerrainGetFinalColor getFinalColor;

    void* _internal_data;
} TerrainRenderer;

extern StandardDefinition TerrainDefinitionClass;
extern StandardRenderer TerrainRendererClass;

void terrainAutoPreset(TerrainDefinition* definition, TerrainPreset preset);
void terrainRenderSurface(Renderer* renderer);
/*Renderer terrainCreatePreviewRenderer();
Color terrainGetPreview(Renderer* renderer, double x, double y);*/

#ifdef __cplusplus
}
#endif

#endif
