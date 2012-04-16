#include "basepreview.h"

#include <math.h>
#include <QVector>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>

class PreviewDrawer:public QThread
{
public:
    PreviewDrawer(BasePreview* preview):
        QThread(),
        _preview(preview)
    {
        _running = false;
    }
    void askStop()
    {
        _running = false;
    }
    static inline void usleep(int us)
    {
        QThread::usleep(us);
    }
protected:
    void run()
    {
        _running = true;
        while (_running)
        {
            _preview->doRender();
            QThread::usleep(50000);
        }
    }
private:
    BasePreview* _preview;
    bool _running;
};

BasePreview::BasePreview(QWidget* parent) :
    QWidget(parent)
{
    this->lock_drawing = new QMutex();

    this->conf_scroll_xmin = 0.0;
    this->conf_scroll_xmax = 0.0;
    this->conf_scroll_xinit = 0.0;
    this->conf_scroll_ymin = 0.0;
    this->conf_scroll_ymax = 0.0;
    this->conf_scroll_yinit = 0.0;
    this->conf_scale_min = 1.0;
    this->conf_scale_max = 1.0;
    this->conf_scale_init = 1.0;
    this->conf_scale_step = 0.0;
    this->scaling = 1.0;
    this->xoffset = 0.0;
    this->yoffset = 0.0;
    this->pixbuf = new QImage(this->size(), QImage::Format_ARGB32);
    this->pixbuf->fill(0x00000000);

    this->alive = true;
    this->need_render = true;

    QObject::connect(this, SIGNAL(contentChange()), this, SLOT(update()));
    QObject::connect(this, SIGNAL(redrawRequested()), this, SLOT(handleRedraw()));

    this->setMinimumSize(256, 256);
    this->setMaximumSize(256, 256);
    this->resize(256, 256);

    this->updater = new PreviewDrawer(this);
}

BasePreview::~BasePreview()
{
    alive = false;

    ((PreviewDrawer*)updater)->askStop();
    updater->wait();

    delete updater;
    delete pixbuf;
    delete lock_drawing;
}

void BasePreview::updateData()
{
}

QColor BasePreview::getColor(double x, double y)
{
    return QColor(0, 0, 0);
}

void BasePreview::configScaling(double min, double max, double step, double init)
{
    double size = (double)width();
    
    if (size >= 1.0)
    {
        conf_scale_min = min / size;
        conf_scale_max = max / size;
        conf_scale_step = step / size;
        conf_scale_init = init / size;
        scaling = init / size;
        redraw();
    }
}

void BasePreview::configScrolling(double xmin, double xmax, double xinit, double ymin, double ymax, double yinit)
{
    conf_scroll_xmin = xmin;
    conf_scroll_xmax = xmax;
    conf_scroll_xinit = xinit;
    conf_scroll_ymin = ymin;
    conf_scroll_ymax = ymax;
    conf_scroll_yinit = yinit;
    xoffset = xinit;
    yoffset = yinit;
    redraw();
}

void BasePreview::start()
{
    this->updater->start();
}

void BasePreview::doRender()
{
    if (this->alive)
    {
        if (this->need_render)
        {
            this->need_render = false;
            this->renderPixbuf();
            emit contentChange();
        }
    }
}

void BasePreview::redraw()
{
    emit(redrawRequested());
}

void BasePreview::handleRedraw()
{
    lock_drawing->lock();
    
    updateData();
    
    QImage part = pixbuf->copy();
    pixbuf->fill(0x00000000);
    QPainter painter(pixbuf);
    painter.setOpacity(0.99);
    painter.drawImage(0, 0, part);
    
    need_render = true;
    
    lock_drawing->unlock();
}

void BasePreview::resizeEvent(QResizeEvent* event)
{
    QImage* image;

    this->lock_drawing->lock();

    image = this->pixbuf;

    this->pixbuf = new QImage(this->size(), QImage::Format_ARGB32);

    this->pixbuf->fill(0x00000000);
    this->need_render = true;

    delete image;

    this->lock_drawing->unlock();
}

void BasePreview::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawImage(0, 0, *this->pixbuf);
}

void BasePreview::renderPixbuf()
{
    QColor col;
    bool done;
    int x, y, w, h;
    QImage* current_pixbuf;

    w = this->pixbuf->width();
    h = this->pixbuf->height();
    current_pixbuf = this->pixbuf;

    for (x = 0; x < w; x++)
    {
        this->lock_drawing->lock();

        if (!this->alive || this->pixbuf != current_pixbuf)
        {
            this->lock_drawing->unlock();
            return;
        }

        done = false;
        for (y = 0; y < h; y++)
        {
            if (!this->alive || this->pixbuf != current_pixbuf)
            {
                this->lock_drawing->unlock();
                return;
            }

            if (qAlpha(this->pixbuf->pixel(x, y)) < 255)
            {
                col = this->getColor((double)(x - w / 2) * this->scaling + this->xoffset, (double)(y - h / 2) * this->scaling + this->yoffset);
                col.setAlpha(255);
                this->pixbuf->setPixel(x, y, col.rgb());
                done = true;
            }
        }
        if (done && (x == w - 1 || x % 10 == 0))
        {
            emit contentChange();
        }
        this->lock_drawing->unlock();
        PreviewDrawer::usleep(100);
    }
}

void BasePreview::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mousex = event->x();
        mousey = event->y();
    }
}

void BasePreview::mouseMoveEvent(QMouseEvent* event)
{
    int dx, dy;
    int ndx, ndy;
    int width, height;
    
    if (event->buttons() & Qt::LeftButton)
    {
        dx = event->x() - mousex;
        dy = event->y() - mousey;

        ndx = dx;
        ndy = dy;
        if (xoffset - dx * scaling > conf_scroll_xmax)
        {
            ndx = (int)floor((conf_scroll_xmax - xoffset) / scaling);
        }
        if (xoffset - dx * scaling < conf_scroll_xmin)
        {
            ndx = (int)floor((conf_scroll_xmin - xoffset) / scaling);
        }
        if (yoffset - dy * scaling > conf_scroll_ymax)
        {
            ndy = (int)floor((conf_scroll_ymax - yoffset) / scaling);
        }
        if (yoffset - dy * scaling < conf_scroll_ymin)
        {
            ndy = (int)floor((conf_scroll_ymin - yoffset) / scaling);
        }
        if (ndx != 0 || ndy != 0)
        {
            width = this->width();
            height = this->height();
            
            if (ndx <= -width || ndx >= width || ndy <= -height || ndy >= height)
            {
                xoffset -= (double)ndx * scaling;
                yoffset -= (double)ndy * scaling;
            
                lock_drawing->lock();
                pixbuf->fill(0x00000000);
                need_render = true;
                lock_drawing->unlock();
            }
            else
            {
                int xstart, xsize, ystart, ysize;
                
                lock_drawing->lock();

                xoffset -= (double)ndx * scaling;
                yoffset -= (double)ndy * scaling;
            
                if (ndx < 0)
                {
                    xstart = -ndx;
                    xsize = width + ndx;
                }
                else
                {
                    xstart = 0;
                    xsize = width - ndx;
                }
                if (ndy < 0)
                {
                    ystart = -ndy;
                    ysize = height + ndy;
                }
                else
                {
                    ystart = 0;
                    ysize = height - ndy;
                }
                
                QImage part = pixbuf->copy(xstart, ystart, xsize, ysize);
                QPainter painter(pixbuf);
                pixbuf->fill(0x00000000);
                painter.drawImage(xstart + ndx, ystart + ndy, part);
                
                need_render = true;
                lock_drawing->unlock();
                
                emit contentChange();
            }
        }

        mousex = event->x();
        mousey = event->y();
    }
}

void BasePreview::wheelEvent(QWheelEvent* event)
{
    double factor;
    double old_scaling;
    int width, height;
    int new_width, new_height;
    
    if (event->modifiers() & Qt::ShiftModifier)
    {
        factor = 5.0;
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
        factor = 0.2;
    }
    else
    {
        factor = 1.0;
    }

    old_scaling = scaling;
    width = pixbuf->width();
    height = pixbuf->height();
    
    if (event->orientation() == Qt::Vertical)
    {
        if (event->delta() > 0  && scaling > conf_scale_min)
        {
            scaling -= conf_scale_step * factor;
            if (scaling < conf_scale_min)
            {
                scaling = conf_scale_min;
            }
            
            lock_drawing->lock();
            new_width = (int)floor(((double)width) * scaling / old_scaling);
            new_height = (int)floor(((double)height) * scaling / old_scaling);
            QImage part = pixbuf->copy((width - new_width) / 2, (height - new_height) / 2, new_width, new_height).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QPainter painter(pixbuf);
            pixbuf->fill(0x00000000);
            painter.setOpacity(0.99);
            painter.drawImage(0, 0, part);
            need_render = true;
            lock_drawing->unlock();
            
            emit contentChange();
        }
        else if (event->delta() < 0  && scaling < conf_scale_max)
        {
            scaling += conf_scale_step * factor;
            if (scaling > conf_scale_max)
            {
                scaling = conf_scale_max;
            }
            
            lock_drawing->lock();
            QImage part = pixbuf->scaled((int)floor(((double)width) * old_scaling / scaling), (int)floor(((double)height) * old_scaling / scaling), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QPainter painter(pixbuf);
            pixbuf->fill(0x00000000);
            painter.setOpacity(0.99);
            painter.drawImage((width - part.width()) / 2, (height - part.height()) / 2, part);
            need_render = true;
            lock_drawing->unlock();
            
            emit contentChange();
        }
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
