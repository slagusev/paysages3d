#include "widgetcurveeditor.h"

#include <QPainter>
#include <QMouseEvent>
#include <qt4/QtGui/qpaintengine.h>
#include "../lib_paysages/tools.h"

WidgetCurveEditor::WidgetCurveEditor(QWidget *parent, double xmin, double xmax, double ymin, double ymax) : QWidget(parent)
{
    _curve = curveCreate();
    _dragged = -1;
    _pen = QColor(0, 0, 0);
    
    setMinimumSize(300, 300);
}

WidgetCurveEditor::~WidgetCurveEditor()
{
    curveDelete(_curve);
}

void WidgetCurveEditor::setAxisLabels(QString xlabel, QString ylabel)
{
    _xlabel = xlabel;
    _ylabel = ylabel;
}

void WidgetCurveEditor::setCurve(Curve* curve)
{
    curveCopy(curve, _curve);
    update();
}

void WidgetCurveEditor::getCurve(Curve* curve)
{
    curveCopy(_curve, curve);
}

void WidgetCurveEditor::setPenColor(QColor color)
{
    _pen = color;
    update();
}

void WidgetCurveEditor::paintEvent(QPaintEvent* event)
{
    int i, n;
    int width, height;
    double dwidth, dheight;
    CurvePoint point;
    double position, value, prev_value, next_value;
    
    width = this->width();
    height = this->height();
    dheight = (double)(height - 1);
    dwidth = (double)(width - 1);
    
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    
    // Draw grid
    painter.setPen(QPen(Qt::lightGray));
    for (int x = 0; x <= 10; x++)
    {
        int vx = (x == 10) ? width - 1 : x * width / 10;
        painter.drawLine(vx, 0, vx, height -1);
        vx = (x == 10) ? height - 1 : x * height / 10;
        painter.drawLine(0, vx, width - 1, vx);
    }
    
    // Draw labels
    painter.setPen(QColor(50, 50, 50));
    if (not _xlabel.isEmpty())
    {
        painter.drawText(rect(), Qt::AlignCenter | Qt::AlignBottom, _xlabel);
    }
    if (not _ylabel.isEmpty())
    {
        painter.rotate(-90.0);
        painter.translate(-height, 0.0);
        painter.drawText(0, 0, height, 20, Qt::AlignCenter | Qt::AlignTop, _ylabel);
        painter.translate(height, 0.0);
        painter.rotate(90.0);
    }
    
    // Draw curve path
    painter.setPen(_pen);
    for (int x = 0; x < width; x++)
    {
        position = ((double)x) / dwidth;
        
        value = curveGetValue(_curve, position);
        prev_value = curveGetValue(_curve, position - 1.0 / dwidth);
        next_value = curveGetValue(_curve, position + 1.0 / dwidth);
        
        painter.drawLine(x, height - 1 - (int)((value + (prev_value - value) / 2.0) * dheight), x, height - 1 - (int)((value + (next_value - value) / 2.0) * dheight));
        painter.drawPoint(x, height - 1 - (int)(value * dheight));
    }
    
    // Draw handles
    n = curveGetPointCount(_curve);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing, true);
    for (i = 0; i < n; i++)
    {
        curveGetPoint(_curve, i, &point);
        painter.drawEllipse(QPointF((int)(point.position * dwidth), height - 1 - (int)(point.value * dheight)), 4.0, 4.0);
    }
}

void WidgetCurveEditor::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _dragged < 0)
    {
        _dragged = getPointAt(event->x(), event->y());
    }
    
    event->accept();
}

void WidgetCurveEditor::mouseMoveEvent(QMouseEvent* event)
{
    CurvePoint point;
    
    if (_dragged >= 0 && (event->buttons() & Qt::LeftButton))
    {
        point.position = ((double)event->x()) / (double)(width() - 1);
        point.value = 1.0 - ((double)event->y()) / (double)(height() - 1);
        
        point.position = (point.position < 0.0) ? 0.0 : point.position;
        point.position = (point.position > 1.0) ? 1.0 : point.position;
        point.value = (point.value < 0.0) ? 0.0 : point.value;
        point.value = (point.value > 1.0) ? 1.0 : point.value;
        
        curveSetPoint(_curve, _dragged, &point);

        update();
        
        emit liveChanged();
    }
    
    event->accept();
}

void WidgetCurveEditor::mouseReleaseEvent(QMouseEvent* event)
{
    int clicked;
    
    if (event->button() == Qt::RightButton)
    {
        clicked = getPointAt(event->x(), event->y());
        if (clicked >= 0)
        {
            curveRemovePoint(_curve, clicked);
            update();
            emit liveChanged();
        }
    }
    else if (event->button() == Qt::LeftButton && _dragged >= 0)
    {
        _dragged = -1;
        curveValidate(_curve);
        update();
    }
    
    event->accept();
}

void WidgetCurveEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    CurvePoint point;
    
    if (event->button() == Qt::LeftButton && _dragged < 0)
    {
        if (getPointAt(event->x(), event->y()) < 0)
        {
            point.position = ((double)event->x()) / (double)(width() - 1);
            point.value = 1.0 - ((double)event->y()) / (double)(height() - 1);

            curveAddPoint(_curve, &point);
            curveValidate(_curve);
            update();
            emit liveChanged();
        }
    }
}

int WidgetCurveEditor::getPointAt(int x, int y)
{
    int n;
    int nearest;
    double distance, ndistance;
    CurvePoint point;
    double dx = ((double)x) / (double)(width() - 1);
    double dy = 1.0 - ((double)y) / (double)(height() - 1);

    n = curveGetPointCount(_curve);
    if (n < 1)
    {
        return -1;
    }

    // Find nearest point
    nearest = -1;
    distance = 0.0;
    for (int i = 0; i < n; i++)
    {
        curveGetPoint(_curve, i, &point);
        ndistance = toolsGetDistance2D(point.position, point.value, dx, dy);
        if (nearest < 0 || ndistance < distance)
        {
            distance = ndistance;
            nearest = i;
        }
    }
    
    if (nearest >= 0 && distance < 0.015)
    {
        return nearest;
    }
    else
    {
        return -1;
    }
}
