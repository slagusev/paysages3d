#include "previewmaterial.h"

#include <math.h>
#include <QPainter>
#include "tools.h"

#include "../lib_paysages/lighting.h"
#include "../lib_paysages/color.h"

/***** Small static preview *****/

SmallMaterialPreview::SmallMaterialPreview(QWidget* parent, SurfaceMaterial* material) : QWidget(parent)
{
    LightDefinition light;

    _lighting = lightingCreateDefinition();
    light.color = COLOR_WHITE;
    light.amplitude = 0.0;
    light.direction.x = -0.5;
    light.direction.y = -0.5;
    light.direction.z = -0.5;
    light.direction = v3Normalize(light.direction);
    light.filtered = 0;
    light.masked = 0;
    light.reflection = 1.0;
    lightingAddLight(&_lighting, light);
    lightingValidateDefinition(&_lighting);

    _material = material;

    _renderer = rendererCreate();
    _renderer.camera_location.x = 0.0;
    _renderer.camera_location.x = 0.0;
    _renderer.camera_location.z = 10.0;
}

SmallMaterialPreview::~SmallMaterialPreview()
{
    rendererDelete(&_renderer);
}

QColor SmallMaterialPreview::getColor(float x, float y)
{
    float dist = sqrt(x * x + y * y);
    Vector3 point;
    Color color;

    if (dist >= 1.0)
    {
        return colorToQColor(COLOR_TRANSPARENT);
    }
    else
    {
        point.x = x;
        point.y = -y;
        if (dist == 0)
        {
            point.z = 1.0;
        }
        else
        {
            point.z = fabs(acos(dist));
        }

        point = v3Normalize(point);
        color = lightingApplyToSurface(&_lighting, &_renderer, point, point, *_material);
        if (dist > 0.95)
        {
            color.a = (1.0 - dist) / 0.05;
        }
        return colorToQColor(color);
    }
}

void SmallMaterialPreview::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    int width = this->width();
    int height = this->height();
    float factor, dx, dy;
    
    if (width > height)
    {
        factor = 2.0 / (float)height;
    }
    else
    {
        factor = 2.0 / (float)width;
    }
    dx = factor * (float)width / 2.0;
    dy = factor * (float)height / 2.0;

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            painter.setPen(getColor((float)x * factor - dx, (float)y * factor - dy));
            painter.drawPoint(x, y);
        }
    }
}

/***** Large dynamic preview *****/

PreviewMaterial::PreviewMaterial(QWidget* parent, SurfaceMaterial* material) : BasePreview(parent)
{
    _small = new SmallMaterialPreview(this, material);
    _small->hide();
    
    configScaling(0.05, 2.0, 0.05, 2.0);
}

PreviewMaterial::~PreviewMaterial()
{
    delete _small;
}

QColor PreviewMaterial::getColor(float x, float y)
{
    return _small->getColor(x, y);
}