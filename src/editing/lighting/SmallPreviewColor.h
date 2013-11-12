#ifndef SMALLPREVIEWCOLOR_H
#define	SMALLPREVIEWCOLOR_H

#include "DrawingWidget.h"

#include "tools/color.h"

class SmallPreviewColor: public DrawingWidget
{
    Q_OBJECT
public:
    SmallPreviewColor(QWidget* parent, Color* color = 0);
    void setColor(Color* color);
protected:
    virtual void doDrawing(QPainter* painter);
private:
    Color* _color;
};

#endif	/* SMALLPREVIEWCOLOR_H */

