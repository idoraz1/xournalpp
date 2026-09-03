#pragma once

#include <vector>

#include "model/PageRef.h"
#include "BaseShapeHandler.h"

class Control;

class XTScatterGraphHandler: public BaseShapeHandler {
public:
    XTScatterGraphHandler(Control* control, const PageRef& page, bool flipShift = false, bool flipControl = false);
    ~XTScatterGraphHandler() override;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;
};
