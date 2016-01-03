#ifndef SCENERY_H
#define SCENERY_H

#include "definition_global.h"

#include "DefinitionNode.h"

namespace paysages {
namespace definition {

/**
 * Global scenery management
 *
 * This class contains the whole scenery definition.
 */
class DEFINITIONSHARED_EXPORT Scenery : public DefinitionNode {
  public:
    typedef enum {
        FILE_OPERATION_OK,
        FILE_OPERATION_IOERROR,
        FILE_OPERATION_APP_MISMATCH,
        FILE_OPERATION_VERSION_MISMATCH
    } FileOperationResult;

    typedef void (*SceneryCustomDataCallback)(PackStream *stream, void *data);

  public:
    Scenery();

    virtual void validate() override;

    FileOperationResult saveGlobal(const string &filepath) const;
    FileOperationResult loadGlobal(const string &filepath);

    /**
     * Undo the last scenery change.
     */
    void undo();
    /**
     * Redo the last scenery change.
     */
    void redo();

    void set(const string &path, const int &value);
    void set(const string &path, const double &value);

    virtual const Scenery *getScenery() const override;

    void autoPreset(RandomGenerator &random = RandomGeneratorDefault);
    void autoPreset(unsigned int seed);

    void setAtmosphere(AtmosphereDefinition *atmosphere);
    inline AtmosphereDefinition *getAtmosphere() const {
        return atmosphere;
    }
    void getAtmosphere(AtmosphereDefinition *atmosphere);

    void setCamera(CameraDefinition *camera);
    inline CameraDefinition *getCamera() const {
        return camera;
    }
    void getCamera(CameraDefinition *camera);

    void setClouds(CloudsDefinition *clouds);
    inline CloudsDefinition *getClouds() const {
        return clouds;
    }
    void getClouds(CloudsDefinition *clouds);

    void setTerrain(TerrainDefinition *terrain);
    inline TerrainDefinition *getTerrain() const {
        return terrain;
    }
    void getTerrain(TerrainDefinition *terrain);

    void setTextures(TexturesDefinition *textures);
    inline TexturesDefinition *getTextures() const {
        return textures;
    }
    void getTextures(TexturesDefinition *textures);

    void setVegetation(VegetationDefinition *Vegetation);
    inline VegetationDefinition *getVegetation() const {
        return vegetation;
    }
    void getVegetation(VegetationDefinition *Vegetation);

    void setWater(WaterDefinition *water);
    inline WaterDefinition *getWater() const {
        return water;
    }
    void getWater(WaterDefinition *water);

    void keepCameraAboveGround(CameraDefinition *camera);

  private:
    AtmosphereDefinition *atmosphere;
    CameraDefinition *camera;
    CloudsDefinition *clouds;
    TerrainDefinition *terrain;
    TexturesDefinition *textures;
    VegetationDefinition *vegetation;
    WaterDefinition *water;
};
}
}

#endif // SCENERY_H
