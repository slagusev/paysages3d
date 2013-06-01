#ifndef MAINTERRAINFORM_H
#define MAINTERRAINFORM_H

#include <QWidget>
#include "common/freeformhelper.h"
#include "rendering/terrain/public.h"

namespace Ui {
class MainTerrainForm;
}

class MainTerrainForm : public QWidget
{
    Q_OBJECT
    
public:
    explicit MainTerrainForm(QWidget *parent = 0);
    ~MainTerrainForm();

    inline TerrainDefinition* getTerrainDefinition() {return _terrain;}

public slots:
    void refreshFromLocalData();
    void refreshFromFellowData();
    void updateLocalDataFromScenery();
    void commitLocalDataToScenery();
    void alterRenderer(Renderer* renderer);

    void buttonPaintingPressed();
    void buttonTexturesPressed();

private:
    Ui::MainTerrainForm *ui;
    FreeFormHelper* _form_helper;

    TerrainDefinition* _terrain;

    PreviewRenderer* _renderer_shape;
};

#endif // MAINTERRAINFORM_H