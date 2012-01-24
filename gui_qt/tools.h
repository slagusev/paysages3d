#ifndef _PAYSAGES_QT_TOOLS_H_
#define _PAYSAGES_QT_TOOLS_H_

#include <QColor>

#include "../lib_paysages/shared/types.h"
#include "../lib_paysages/shared/functions.h"

static inline QColor colorToQColor(Color color)
{
    colorNormalize(&color);
    return QColor(color.r * 255.0, color.g * 255.0, color.b * 255.0, color.a * 255.0);
}

#endif
