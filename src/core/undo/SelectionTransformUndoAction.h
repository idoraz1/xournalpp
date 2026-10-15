/*
 * Xournal++
 *
 * Undo action for the discrete selection transforms: rotation by 90/180 degrees and mirroring
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string
#include <vector>  // for vector

#include "model/PageRef.h"  // for PageRef

#include "UndoAction.h"  // for UndoAction

class Control;
class Element;

enum class SelectionTransform { ROTATE_CW, ROTATE_CCW, ROTATE_180, FLIP_HORIZONTAL, FLIP_VERTICAL };

/**
 * Applies the transform around (cx, cy).
 * Strokes are really rotated/mirrored. Text, images and LaTeX cannot be rotated or mirrored, so they keep their
 * orientation and only move to the transformed position of their center.
 * The document must be locked by the caller.
 */
void applySelectionTransform(SelectionTransform t, const std::vector<Element*>& elements, double cx, double cy);

class SelectionTransformUndoAction: public UndoAction {
public:
    SelectionTransformUndoAction(const PageRef& page, std::vector<Element*> elements, double cx, double cy,
                                 SelectionTransform transform);

public:
    bool undo(Control* control) override;
    bool redo(Control* control) override;
    std::string getText() override;

private:
    void apply(SelectionTransform t, Control* control);

private:
    std::vector<Element*> elements;
    double cx;
    double cy;
    SelectionTransform transform;
};
