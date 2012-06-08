#include "explorerchunkterrain.h"

#include <math.h>
#include "baseexplorerchunk.h"
#include "../lib_paysages/camera.h"

ExplorerChunkTerrain::ExplorerChunkTerrain(Renderer* renderer, double x, double z, double size, int nbchunks) : BaseExplorerChunk(renderer)
{
    _startx = x;
    _startz = z;
    _size = size;
    _overall_step = size * (double)nbchunks;
    
    _tessellation_max_size = 32;
    _tessellation = new double[(_tessellation_max_size + 1) * (_tessellation_max_size + 1)];
    _tessellation_current_size = 0;
    _tessellation_step = _size / (double)_tessellation_max_size;
    
    setMaxTextureSize(128);
    
    maintain();
}

ExplorerChunkTerrain::~ExplorerChunkTerrain()
{
    _lock_data.lock();
    delete [] _tessellation;
    _lock_data.unlock();
}

void ExplorerChunkTerrain::onResetEvent()
{
    _tessellation_current_size = 0;
}

bool ExplorerChunkTerrain::onMaintainEvent()
{
    Renderer* renderer = this->renderer();
    
    // Improve heightmap resolution
    if (_tessellation_current_size < _tessellation_max_size)
    {
        int new_tessellation_size = _tessellation_current_size ? _tessellation_current_size * 4 : 2;
        int old_tessellation_inc = _tessellation_current_size ? _tessellation_max_size / _tessellation_current_size : 1;
        int new_tessellation_inc = _tessellation_max_size / new_tessellation_size;
        for (int j = 0; j <= _tessellation_max_size; j += new_tessellation_inc)
        {
            for (int i = 0; i <= _tessellation_max_size; i += new_tessellation_inc)
            {
                if (_tessellation_current_size == 0 || i % old_tessellation_inc != 0 || j % old_tessellation_inc != 0)
                {
                    double height = renderer->getTerrainHeight(renderer, _startx + _tessellation_step * (double)i, _startz + _tessellation_step * (double)j);
                    _tessellation[j * (_tessellation_max_size + 1) + i] = height;
                }
            }
        }

        _lock_data.lock();
        _tessellation_current_size = new_tessellation_size;
        _lock_data.unlock();

        return true;
    }
    else
    {
        return false;
    }
}

void ExplorerChunkTerrain::onCameraEvent(CameraDefinition* camera)
{
    // Handle position
    _lock_data.lock();
    if (camera->location.x > _startx + _overall_step * 0.5)
    {
        _startx += _overall_step;
        askReset();
    }
    if (camera->location.z > _startz + _overall_step * 0.5)
    {
        _startz += _overall_step;
        askReset();
    }
    if (camera->location.x < _startx - _overall_step * 0.5)
    {
        _startx -= _overall_step;
        askReset();
    }
    if (camera->location.z < _startz - _overall_step * 0.5)
    {
        _startz -= _overall_step;
        askReset();
    }
    _lock_data.unlock();
}

void ExplorerChunkTerrain::onRenderEvent(QGLWidget* widget)
{
    _lock_data.lock();
    int tessellation_size = _tessellation_current_size;
    double tsize = 1.0 / (double)_tessellation_max_size;
    _lock_data.unlock();
    
    if (tessellation_size == 0)
    {
        return;
    }

    int tessellation_inc = _tessellation_max_size / (double)tessellation_size;
    for (int j = 0; j < _tessellation_max_size; j += tessellation_inc)
    {
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= _tessellation_max_size; i += tessellation_inc)
        {
            glTexCoord2d(tsize * (double)i, 1.0 - tsize * (double)j);
            glVertex3d(_startx + _tessellation_step * (double)i, _tessellation[j * (_tessellation_max_size + 1) + i], _startz + _tessellation_step * (double)j);
            glTexCoord2d(tsize * (double)i, 1.0 - tsize * (double)(j + tessellation_inc));
            glVertex3d(_startx + _tessellation_step * (double)i, _tessellation[(j + tessellation_inc) * (_tessellation_max_size + 1) + i], _startz + _tessellation_step * (double)(j + tessellation_inc));
        }
        glEnd();
    }
}

double ExplorerChunkTerrain::getDisplayedSizeHint(CameraDefinition* camera)
{
    double distance, wanted_size;
    Vector3 center;

    center = getCenter();

    distance = v3Norm(v3Sub(camera->location, center));
    distance = distance < 0.1 ? 0.1 : distance;
    wanted_size = (int)ceil(120.0 - distance / 3.0);

    if (!cameraIsBoxInView(camera, center, _size, _size, 40.0))
    {
        wanted_size -= 500.0;
    }

    return wanted_size;
}

Color ExplorerChunkTerrain::getTextureColor(double x, double y)
{
    Vector3 location = {_startx + x * _size, 0.0, _startz + y * _size};
    return renderer()->applyTextures(renderer(), location, 0.01);
}

Vector3 ExplorerChunkTerrain::getCenter()
{
    Vector3 result;
    
    result.x = _startx + _size / 2.0;
    result.y = 0.0;
    result.z = _startz + _size / 2.0;
    
    return result;
}
