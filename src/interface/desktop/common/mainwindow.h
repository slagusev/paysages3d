#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "desktop_global.h"

#include <QMainWindow>

#include <QVector>
class FormRender;

namespace Ui {
class MainWindow;
}

class MainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

    virtual bool event(QEvent* event);

    void guiSave(PackStream* stream);
    void guiLoad(PackStream* stream);

    static inline MainWindow* instance()
    {
        return _instance;
    }

protected:

public slots:
    void refreshAll();

    void openTab(int position);

    void fileNew();
    void fileSave();
    void fileLoad();

    void showAboutDialog();

    void quickPreview();
    void finalRender();
    void showLastRender();

    void explore3D();

signals:
    void refreshed();

private:
    static MainWindow* _instance;
    Ui::MainWindow *ui;

    QVector<BaseForm*> _forms;
    FormRender* _form_render;
};

#endif // MAINWINDOW_H
