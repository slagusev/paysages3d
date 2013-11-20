#ifndef _PAYSAGES_QT_DIALOGRENDER_H_
#define _PAYSAGES_QT_DIALOGRENDER_H_

#include <ctime>
#include <QDialog>
#include "renderer.h"

class QThread;
class QProgressBar;
class QSlider;
class QComboBox;
class QLabel;
class QScrollArea;
class QMutex;

class DialogRender : public QDialog
{
    Q_OBJECT
public:
    explicit DialogRender(QWidget *parent, Renderer* renderer);
    ~DialogRender();

    void tellRenderSize(int width, int height);
    void tellProgressChange(double value);
    void tellRenderEnded();
    void startRender(RenderParams params);
    void loadLastRender();

    QImage* pixbuf;
    QMutex* pixbuf_lock;
    QWidget* area;

private slots:
    void applyRenderSize(int width, int height);
    void applyProgress(double value);
    void saveRender();
    void applyRenderEnded();
    void toneMappingChanged();

signals:
    void renderSizeChanged(int width, int height);
    void progressChanged(double value);
    void renderEnded();

private:
    QScrollArea* _scroll;
    QWidget* _info;
    QWidget* _actions;
    QComboBox* _tonemapping_control;
    QSlider* _exposure_control;
    QPushButton* _save_button;
    QThread* _render_thread;
    QLabel* _timer;
    Renderer* _renderer;
    QProgressBar* _progress;
    time_t _started;
};

#endif