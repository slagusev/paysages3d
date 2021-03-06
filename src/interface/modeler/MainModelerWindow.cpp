#include "MainModelerWindow.h"

#include "AtmosphereModeler.h"
#include "Logs.h"
#include "ModelerCameras.h"
#include "OpenGLRenderer.h"
#include "OpenGLView.h"
#include "RenderConfig.h"
#include "RenderPreviewProvider.h"
#include "RenderProcess.h"
#include "Scenery.h"
#include "WaterModeler.h"

#include <QGuiApplication>
#include <QQmlEngine>

MainModelerWindow::MainModelerWindow() {
    QSurfaceFormat new_format = format();
    new_format.setVersion(OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION);
    new_format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(new_format);

    scenery = new Scenery();
    scenery->autoPreset();

    renderer = new OpenGLRenderer(scenery);

    render_preview_provider = new RenderPreviewProvider();

    qmlRegisterType<OpenGLView>("Paysages", 1, 0, "OpenGLView");
    engine()->addImageProvider("renderpreviewprovider", render_preview_provider);

    setMinimumSize(QSize(1280, 720));
    setTitle(QObject::tr("Paysages 3D"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setSource(QUrl("qrc:///main.qml"));

    atmosphere = new AtmosphereModeler(this);
    water = new WaterModeler(this);
    cameras = new ModelerCameras(this);

    render_process = new RenderProcess(this, render_preview_provider);

    connectQmlSignal("tool_file_new", SIGNAL(clicked()), this, SLOT(newFile()));
    connectQmlSignal("tool_file_save", SIGNAL(clicked()), this, SLOT(saveFile()));
    connectQmlSignal("tool_file_load", SIGNAL(clicked()), this, SLOT(loadFile()));
    connectQmlSignal("tool_file_exit", SIGNAL(clicked()), this, SLOT(exit()));
    connectQmlSignal("root", SIGNAL(stopped()), this, SLOT(effectiveExit()));
}

MainModelerWindow::~MainModelerWindow() {
    // TODO Should be done automatically for all tools
    cameras->unregister();
    atmosphere->destroy();
    water->destroy();

    delete atmosphere;
    delete water;
    delete cameras;

    // delete render_preview_provider;  // don't delete it, addImageProvider took ownership
    delete render_process;

    delete renderer;
    delete scenery;
}

QObject *MainModelerWindow::findQmlObject(const string &objectName) {
    if (objectName == "ui" || objectName == "root") {
        return rootObject();
    } else {
        return rootObject()->findChild<QObject *>(QString::fromStdString(objectName));
    }
}

void MainModelerWindow::setQmlProperty(const string &objectName, const string &propertyName, const QVariant &value) {
    QObject *item = findQmlObject(objectName);
    if (item) {
        item->setProperty(propertyName.c_str(), value);
    } else {
        Logs::error("UI") << "QML object not found :" << objectName << endl;
    }
}

void MainModelerWindow::connectQmlSignal(const string &objectName, const char *signal, const QObject *receiver,
                                         const char *method) {
    QObject *item = findQmlObject(objectName);
    if (item) {
        connect(item, signal, receiver, method);
    } else {
        Logs::error("UI") << "QML object not found :" << objectName << endl;
    }
}

string MainModelerWindow::getState() const {
    return rootObject()->property("state").toString().toStdString();
}

void MainModelerWindow::setState(const string &stateName) {
    rootObject()->setProperty("state", QString::fromStdString(stateName));
}

void MainModelerWindow::newFile() {
    getScenery()->autoPreset();
    renderer->reset();
}

void MainModelerWindow::saveFile() {
    getScenery()->saveGlobal("saved.p3d");
}

void MainModelerWindow::loadFile() {
    Scenery loaded;
    if (loaded.loadGlobal("saved.p3d") == Scenery::FILE_OPERATION_OK) {
        loaded.copy(scenery);
        renderer->reset();
    }
}

void MainModelerWindow::exit() {
    renderer->stop();
}

void MainModelerWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_Q) {
        exit();
    } else if (getState() == "Render Dialog") {
        if (event->key() == Qt::Key_Escape) {
            render_process->stopRender();
        }
    } else {
        if (event->key() == Qt::Key_F5) {
            // Start render in a thread
            if ((event->modifiers() & Qt::ControlModifier) and (event->modifiers() & Qt::ShiftModifier)) {
                render_process->startFinalRender();
            } else if (event->modifiers() & Qt::ControlModifier) {
                render_process->startMediumRender();
            } else {
                render_process->startQuickRender();
            }
        } else if (event->key() == Qt::Key_F6) {
            render_process->showPreviousRender();
        } else if (event->key() == Qt::Key_F12) {
            Logs::warning("UI") << "Current scenery dump:" << endl << scenery->toString() << endl;
        } else if (event->key() == Qt::Key_N) {
            if (event->modifiers() & Qt::ControlModifier) {
                newFile();
            }
        } else if (event->key() == Qt::Key_S) {
            if (event->modifiers() & Qt::ControlModifier) {
                saveFile();
            }
        } else if (event->key() == Qt::Key_L or event->key() == Qt::Key_O) {
            if (event->modifiers() & Qt::ControlModifier) {
                loadFile();
            }
        } else if (event->key() == Qt::Key_Z) {
            if (event->modifiers() & Qt::ControlModifier) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    getScenery()->redo();
                } else {
                    getScenery()->undo();
                }
            }
        } else if (event->key() == Qt::Key_Y) {
            if (event->modifiers() & Qt::ControlModifier) {
                getScenery()->redo();
            }
        }
    }
}

void MainModelerWindow::effectiveExit() {
    close();
}
