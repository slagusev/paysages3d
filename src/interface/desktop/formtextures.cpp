#include "formtextures.h"

#include "DesktopScenery.h"
#include "BasePreview.h"
#include "tools.h"
#include "CameraDefinition.h"
#include "TexturesDefinition.h"
#include "TextureLayerDefinition.h"
#include "SoftwareRenderer.h"
#include "TerrainRenderer.h"

/**************** Previews ****************/
class PreviewTexturesCoverage : public BasePreview
{
public:

    PreviewTexturesCoverage(QWidget* parent, TextureLayerDefinition* layer) : BasePreview(parent)
    {
        _renderer = new SoftwareRenderer();
        _renderer->render_quality = 3;

        _original_layer = layer;

        addOsd(QString("geolocation"));

        configScaling(20.0, 500.0, 20.0, 50.0);
        configScrolling(-1000.0, 1000.0, 0.0, -1000.0, 1000.0, 0.0);
    }

    ~PreviewTexturesCoverage()
    {
        delete _renderer;
    }
protected:

    Color getColor(double x, double y)
    {
        Vector3 location;
        Color result;
        location.x = x;
        location.y = _renderer->getTerrainRenderer()->getHeight(x, y, 1);
        location.z = y;
        //result.r = result.g = result.b = texturesGetLayerCoverage(_preview_layer, _renderer, location, this->scaling);
        return result;
    }

    void updateData()
    {
        TexturesDefinition* textures = _renderer->getScenery()->getTextures();
        textures->clear();
        textures->addLayer();
        _original_layer->copy(textures->getLayer(0));

        _renderer->prepare();
    }

private:
    SoftwareRenderer* _renderer;
    TextureLayerDefinition* _original_layer;
};

class PreviewTexturesColor : public BasePreview
{
public:

    PreviewTexturesColor(QWidget* parent, TextureLayerDefinition* layer) : BasePreview(parent)
    {
        _original_layer = layer;

        _renderer = new SoftwareRenderer();
        _renderer->render_quality = 3;

        _renderer->render_camera->setLocation(Vector3(0.0, 20.0, 0.0));

        configScaling(0.01, 1.0, 0.01, 0.1);
        configScrolling(-1000.0, 1000.0, 0.0, -1000.0, 1000.0, 0.0);
    }

    ~PreviewTexturesColor()
    {
        delete _renderer;
    }
protected:

    Color getColor(double x, double y)
    {
        Vector3 location;
        location.x = x;
        location.y = 0.0;
        location.z = y;
        //return texturesGetLayerColor(_preview_layer, _renderer, location, this->scaling);
        return COLOR_BLACK;
    }

    void updateData()
    {
        TexturesDefinition* textures = _renderer->getScenery()->getTextures();
        textures->clear();
        textures->addLayer();
        _original_layer->copy(textures->getLayer(0));

        _renderer->prepare();
    }
private:
    SoftwareRenderer* _renderer;
    TextureLayerDefinition* _original_layer;
};

/**************** Form ****************/
FormTextures::FormTextures(QWidget *parent) :
BaseFormLayer(parent)
{
    addAutoPreset(tr("Rock"));
    addAutoPreset(tr("Grass"));
    addAutoPreset(tr("Sand"));
    addAutoPreset(tr("Snow"));

    _definition = new TexturesDefinition(NULL);
    _layer = new TextureLayerDefinition(NULL);

    _previewCoverage = new PreviewTexturesCoverage(this, _layer);
    _previewColor = new PreviewTexturesColor(this, _layer);
    addPreview(_previewCoverage, tr("Coverage preview"));
    addPreview(_previewColor, tr("Lighted sample"));

    addInputDouble(tr("Displacement height"), &_layer->displacement_height, 0.0, 0.1, 0.001, 0.01);
    addInputDouble(tr("Displacement scaling"), &_layer->displacement_scaling, 0.003, 0.3, 0.003, 0.03);
    addInputMaterial(tr("Material"), _layer->material);
    /*addInputCurve(tr("Coverage by altitude"), _layer->terrain_zone->value_by_height, -20.0, 20.0, 0.0, 1.0, tr("Terrain altitude"), tr("Texture coverage"));
    addInputCurve(tr("Coverage by slope"), _layer->terrain_zone->value_by_slope, 0.0, 5.0, 0.0, 1.0, tr("Terrain slope"), tr("Texture coverage"));*/

    /*addInputDouble(tr("Amplitude for slope coverage"), &_layer->slope_range, 0.001, 0.1, 0.001, 0.01);
    addInputDouble(tr("Layer thickness"), &_layer->thickness, 0.0, 0.1, 0.001, 0.01);
    addInputDouble(tr("Transparency thickness"), &_layer->thickness_transparency, 0.0, 0.1, 0.001, 0.01);*/

    setLayers(_definition);
}

FormTextures::~FormTextures()
{
    delete _definition;
    delete _layer;
}

void FormTextures::revertConfig()
{
    DesktopScenery::getCurrent()->getTextures(_definition);
    BaseFormLayer::revertConfig();
}

void FormTextures::applyConfig()
{
    BaseFormLayer::applyConfig();
    DesktopScenery::getCurrent()->setTextures(_definition);
}

void FormTextures::layerReadCurrentFrom(void* layer_definition)
{
    ((TextureLayerDefinition*)layer_definition)->copy(_layer);
}

void FormTextures::layerWriteCurrentTo(void* layer_definition)
{
    _layer->copy((TextureLayerDefinition*)layer_definition);
}

void FormTextures::autoPresetSelected(int preset)
{
    _layer->applyPreset((TextureLayerDefinition::TextureLayerPreset)preset);
    BaseForm::autoPresetSelected(preset);
}
