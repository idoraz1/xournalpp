/*
 * Xournal++
 *
 * Tab-like strip listing the notebooks opened during this session, to switch between them quickly
 * (e.g. between the lecture summary and the exercises notebook).
 *
 * Xournal++ holds one document at a time: switching saves the current notebook (if it has a file) and
 * opens the other one. The page and zoom of each notebook are restored from the metadata.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <vector>  // for vector

#include <gtk/gtk.h>  // for GtkWidget

#include "util/raii/GObjectSPtr.h"  // for WidgetSPtr

#include "filesystem.h"  // for path

class Control;

class SessionDocumentsBar final {
public:
    explicit SessionDocumentsBar(Control* control);
    ~SessionDocumentsBar();

    GtkWidget* getWidget() const;

    /**
     * Call whenever the current document (or its path) may have changed.
     * Adds the file to the session list if it is new and highlights it.
     */
    void setCurrentDocument(const fs::path& path);

private:
    void rebuild();
    void scheduleRebuild();
    // By value: the caller's path belongs to a button that a rebuild may destroy
    void switchTo(fs::path path);
    void remove(fs::path path);

private:
    Control* control;
    xoj::util::WidgetSPtr box;
    guint rebuildSource = 0;

    std::vector<fs::path> documents;
    fs::path current;
};
