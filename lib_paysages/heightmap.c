#include "heightmap.h"
#include "tools.h"
#include "system.h"
#include "noise.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

HeightMap heightmapCreate()
{
    HeightMap result;
    
    result.data = malloc(sizeof(double));
    result.resolution_x = 1;
    result.resolution_z = 1;
    
    return result;
}

void heightmapDelete(HeightMap* heightmap)
{
    free(heightmap->data);
}

void heightmapCopy(HeightMap* source, HeightMap* destination)
{
    destination->resolution_x = source->resolution_x;
    destination->resolution_z = source->resolution_z;
    destination->data = realloc(destination->data, sizeof(double) * destination->resolution_x * destination->resolution_z);
    memcpy(destination->data, source->data, sizeof(double) * destination->resolution_x * destination->resolution_z);
}

void heightmapValidate(HeightMap* heightmap)
{
}

void heightmapSave(PackStream* stream, HeightMap* heightmap)
{
    int i;
    
    packWriteInt(stream, &heightmap->resolution_x);
    packWriteInt(stream, &heightmap->resolution_z);
    for (i = 0; i < heightmap->resolution_x * heightmap->resolution_z; i++)
    {
        packWriteDouble(stream, &heightmap->data[i]);
    }
}

void heightmapLoad(PackStream* stream, HeightMap* heightmap)
{
    int i;
    
    packReadInt(stream, &heightmap->resolution_x);
    packReadInt(stream, &heightmap->resolution_z);
    heightmap->data = realloc(heightmap->data, sizeof(double) * heightmap->resolution_x * heightmap->resolution_z);
    for (i = 0; i < heightmap->resolution_x * heightmap->resolution_z; i++)
    {
        packReadDouble(stream, &heightmap->data[i]);
    }
}

static void _loadFromFilePixel(HeightMap* heightmap, int x, int y, Color col)
{
    assert(x >= 0 && x < heightmap->resolution_x);
    assert(y >= 0 && y < heightmap->resolution_z);
    
    heightmap->data[y * heightmap->resolution_x + x] = (col.r + col.g + col.b) / 3.0;
}

void heightmapImportFromPicture(HeightMap* heightmap, const char* picturepath)
{
    systemLoadPictureFile(picturepath, (PictureCallbackLoadStarted)heightmapChangeResolution, (PictureCallbackLoadPixel)_loadFromFilePixel, heightmap);
}

void heightmapChangeResolution(HeightMap* heightmap, int resolution_x, int resolution_z)
{
    int i;
    
    heightmap->resolution_x = resolution_x;
    heightmap->resolution_z = resolution_z;
    heightmap->data = realloc(heightmap->data, sizeof(double) * heightmap->resolution_x * heightmap->resolution_z);
    
    for (i = 0; i < heightmap->resolution_x * heightmap->resolution_z; i++)
    {
        heightmap->data[i] = 0.0;
    }
}

void heightmapGetLimits(HeightMap* heightmap, double* ymin, double* ymax)
{
    double y;
    int i;
    *ymin = 1000000.0;
    *ymax = -1000000.0;
    /* TODO Keep the value in cache */
    for (i = 0; i < heightmap->resolution_x * heightmap->resolution_z; i++)
    {
        y = heightmap->data[i];
        if (y < *ymin)
        {
            *ymin = y;
        }
        if (y > *ymax)
        {
            *ymax = y;
        }
    }
}

double heightmapGetRawValue(HeightMap* heightmap, double x, double z)
{
    assert(x >= 0.0 && x <= 1.0);
    assert(z >= 0.0 && z <= 1.0);
    
    return heightmap->data[((int)(z * (double)(heightmap->resolution_z - 1))) * heightmap->resolution_x + ((int)(x * (double)(heightmap->resolution_x - 1)))];
}

double heightmapGetValue(HeightMap* heightmap, double x, double z)
{
    int xmax = heightmap->resolution_x - 1;
    int zmax = heightmap->resolution_z - 1;
    int xlow = floor(x * xmax);
    int zlow = floor(z * zmax);
    double stencil[16];
    int ix, iz, cx, cz;
    
    for (ix = xlow - 1; ix <= xlow + 2; ix++)
    {
        for (iz = zlow - 1; iz <= zlow + 2; iz++)
        {
            cx = ix < 0 ? 0 : ix;
            cx = cx > xmax ? xmax : cx;
            cz = iz < 0 ? 0 : iz;
            cz = cz > zmax ? zmax : cz;
            stencil[(iz - (zlow - 1)) * 4 + ix - (xlow - 1)] = heightmap->data[cz * heightmap->resolution_x + cx];
        }
    }
    
    return toolsBicubicInterpolate(stencil, x * xmax - (double)xlow, z * zmax - (double)zlow);
}

static inline void _getBrushBoundaries(HeightMapBrush* brush, int rx, int rz, int* x1, int* x2, int* z1, int* z2)
{
    double cx = brush->relative_x * rx;
    double cz = brush->relative_z * rz;
    double s = brush->smoothed_size + brush->hard_radius;
    double sx = s * rx;
    double sz = s * rz;
    *x1 = (int)floor(cx - sx);
    *x2 = (int)ceil(cx + sx);
    *z1 = (int)floor(cz - sz);
    *z2 = (int)ceil(cz + sz);
    if (*x1 < 0) 
    {
        *x1 = 0;
    }
    if (*x1 > rx) 
    {
        *x1 = rx;
    }
    if (*z1 < 0) 
    {
        *z1 = 0;
    }
    if (*z1 > rz) 
    {
        *z1 = rz;
    }
}

void heightmapBrushElevation(HeightMap* heightmap, HeightMapBrush* brush, double value)
{
    int x, x1, x2, z, z1, z2;
    double dx, dz, distance;
    
    _getBrushBoundaries(brush, heightmap->resolution_x - 1, heightmap->resolution_z - 1, &x1, &x2, &z1, &z2);
    
    for (x = x1; x <= x2; x++)
    {
        dx = (double)x / (double)heightmap->resolution_x;
        for (z = z1; z <= z2; z++)
        {
            dz = (double)z / (double)heightmap->resolution_z;
            distance = sqrt((brush->relative_x - dx) * (brush->relative_x - dx) + (brush->relative_z - dz) * (brush->relative_z - dz));
            
            if (distance > brush->hard_radius)
            {
                if (distance <= brush->hard_radius + brush->smoothed_size)
                {
                    heightmap->data[z * heightmap->resolution_x + x] += value * (1.0 - (distance - brush->hard_radius) / brush->smoothed_size);
                }
            }
            else
            {
                heightmap->data[z * heightmap->resolution_x + x] += value;
            }
        }
    }
}

void heightmapBrushSmooth(HeightMap* heightmap, HeightMapBrush* brush, double value)
{
    /* TODO */
}

void heightmapBrushAddNoise(HeightMap* heightmap, HeightMapBrush* brush, NoiseGenerator* generator, double value)
{
    int x, x1, x2, z, z1, z2;
    double dx, dz, distance, factor, brush_size;
    
    _getBrushBoundaries(brush, heightmap->resolution_x - 1, heightmap->resolution_z - 1, &x1, &x2, &z1, &z2);
    brush_size = brush->hard_radius + brush->smoothed_size;
    
    for (x = x1; x <= x2; x++)
    {
        dx = (double)x / (double)heightmap->resolution_x;
        for (z = z1; z <= z2; z++)
        {
            dz = (double)z / (double)heightmap->resolution_z;
            distance = sqrt((brush->relative_x - dx) * (brush->relative_x - dx) + (brush->relative_z - dz) * (brush->relative_z - dz));
            
            if (distance > brush->hard_radius)
            {
                if (distance <= brush->hard_radius + brush->smoothed_size)
                {
                    factor = (1.0 - (distance - brush->hard_radius) / brush->smoothed_size);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                factor = 1.0;
            }
            
            heightmap->data[z * heightmap->resolution_x + x] += factor * noiseGet2DTotal(generator, dx / brush_size, dz / brush_size) * brush_size;
        }
    }
}
