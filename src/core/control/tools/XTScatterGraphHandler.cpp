#include "XTScatterGraphHandler.h"

#include <algorithm>

#include "control/Control.h"
#include "control/settings/Settings.h"
#include "control/tools/SnapToGridInputHandler.h"
#include "model/Point.h"

XTScatterGraphHandler::XTScatterGraphHandler(Control* control, const PageRef& page, bool flipShift,
                                             bool flipControl):
        BaseShapeHandler(control, page, flipShift, flipControl) {}

XTScatterGraphHandler::~XTScatterGraphHandler() = default;

auto XTScatterGraphHandler::createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
        -> std::pair<std::vector<Point>, Range> {
    Point c = snappingHandler.snapToGrid(this->currPoint, isAltDown);

    double width = c.x - this->startPoint.x;
    double height = c.y - this->startPoint.y;

    this->modShift = isShiftDown;
    this->modControl = isControlDown;

    Settings* settings = control->getSettings();
    if (settings->getDrawDirModsEnabled()) {
        this->modifyModifiersByDrawDir(width, height, true);
    }

    if (this->modShift) {
        int signW = width > 0 ? 1 : -1;
        int signH = height > 0 ? 1 : -1;
        width = std::max(width * signW, height * signH) * signW;
        height = (width * signW) * signH;
    }

    const Point& p1 = this->startPoint;

    Range rg(p1.x, p1.y - std::abs(height));
    rg.addPoint(p1.x + std::abs(width), p1.y + std::abs(height));

    std::vector<Point> pts;

    // Vertical line
    pts.emplace_back(p1.x, p1.y - std::abs(height));
    pts.emplace_back(p1.x, p1.y + std::abs(height));

    // Horizontal line (disconnected)
    pts.emplace_back(p1.x, p1.y, Point::NO_PRESSURE);
    pts.emplace_back(p1.x + std::abs(width), p1.y);

    return {pts, rg};
}
