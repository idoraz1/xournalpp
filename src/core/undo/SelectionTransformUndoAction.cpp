#include "SelectionTransformUndoAction.h"

#include <cmath>    // for M_PI
#include <utility>  // for move

#include "control/Control.h"  // for Control
#include "model/Document.h"   // for Document
#include "model/Element.h"    // for Element
#include "model/XojPage.h"    // for XojPage
#include "util/Range.h"       // for Range
#include "util/i18n.h"        // for _

namespace {
auto inverse(SelectionTransform t) -> SelectionTransform {
    switch (t) {
        case SelectionTransform::ROTATE_CW:
            return SelectionTransform::ROTATE_CCW;
        case SelectionTransform::ROTATE_CCW:
            return SelectionTransform::ROTATE_CW;
        default:
            return t;  // 180° rotation and mirroring are their own inverse
    }
}

/// Image of the point (x, y) under the transform around (cx, cy)
void transformPoint(SelectionTransform t, double cx, double cy, double& x, double& y) {
    double dx = x - cx;
    double dy = y - cy;
    switch (t) {
        case SelectionTransform::ROTATE_CW:  // y axis points down: clockwise on screen
            x = cx - dy;
            y = cy + dx;
            break;
        case SelectionTransform::ROTATE_CCW:
            x = cx + dy;
            y = cy - dx;
            break;
        case SelectionTransform::ROTATE_180:
            x = cx - dx;
            y = cy - dy;
            break;
        case SelectionTransform::FLIP_HORIZONTAL:
            x = cx - dx;
            break;
        case SelectionTransform::FLIP_VERTICAL:
            y = cy - dy;
            break;
    }
}
}  // namespace

void applySelectionTransform(SelectionTransform t, const std::vector<Element*>& elements, double cx, double cy) {
    for (Element* e: elements) {
        if (e->getType() == ELEMENT_STROKE) {
            switch (t) {
                case SelectionTransform::ROTATE_CW:
                    e->rotate(cx, cy, M_PI / 2);
                    break;
                case SelectionTransform::ROTATE_CCW:
                    e->rotate(cx, cy, -M_PI / 2);
                    break;
                case SelectionTransform::ROTATE_180:
                    // Exact (no floating point drift from sin/cos), line width unchanged since |fx * fy| = 1
                    e->scale(cx, cy, -1, -1, 0, false);
                    break;
                case SelectionTransform::FLIP_HORIZONTAL:
                    e->scale(cx, cy, -1, 1, 0, false);
                    break;
                case SelectionTransform::FLIP_VERTICAL:
                    e->scale(cx, cy, 1, -1, 0, false);
                    break;
            }
        } else {
            const auto& box = e->getBoundingBox();
            double ox = box.x + box.width / 2;
            double oy = box.y + box.height / 2;
            double nx = ox;
            double ny = oy;
            transformPoint(t, cx, cy, nx, ny);
            e->move(nx - ox, ny - oy);
        }
    }
}

SelectionTransformUndoAction::SelectionTransformUndoAction(const PageRef& page, std::vector<Element*> elements,
                                                           double cx, double cy, SelectionTransform transform):
        UndoAction("SelectionTransformUndoAction"),
        elements(std::move(elements)),
        cx(cx),
        cy(cy),
        transform(transform) {
    this->page = page;
}

auto SelectionTransformUndoAction::undo(Control* control) -> bool {
    apply(inverse(transform), control);
    this->undone = true;
    return true;
}

auto SelectionTransformUndoAction::redo(Control* control) -> bool {
    apply(transform, control);
    this->undone = false;
    return true;
}

void SelectionTransformUndoAction::apply(SelectionTransform t, Control* control) {
    if (elements.empty()) {
        return;
    }
    Document* doc = control->getDocument();

    Range r;
    doc->lock();
    for (Element* e: elements) {
        r = r.unite(Range(e->getBoundingBox()));
    }
    applySelectionTransform(t, elements, cx, cy);
    for (Element* e: elements) {
        r = r.unite(Range(e->getBoundingBox()));
    }
    doc->unlock();

    this->page->fireRangeChanged(r);
}

auto SelectionTransformUndoAction::getText() -> std::string {
    switch (transform) {
        case SelectionTransform::FLIP_HORIZONTAL:
        case SelectionTransform::FLIP_VERTICAL:
            return _("Flip");
        default:
            return _("Rotation");
    }
}
