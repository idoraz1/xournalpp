#pragma once

#include <vector>

#include "model/PageRef.h"
#include "BaseShapeHandler.h"

class Control;

class XYScatterGraphHandler: public BaseShapeHandler {
public:
    XYScatterGraphHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~XYScatterGraphHandler() override;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;
};
