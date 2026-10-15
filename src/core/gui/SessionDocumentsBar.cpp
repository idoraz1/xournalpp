#include "SessionDocumentsBar.h"

#include <algorithm>  // for find
#include <string>     // for string

#include <glib/gi18n.h>  // for _

#include "control/Control.h"       // for Control
#include "undo/UndoRedoHandler.h"  // for UndoRedoHandler
#include "util/StringUtils.h"      // for char_cast
#include "util/glib_casts.h"       // for closure_notify_cb

namespace {
/// Data attached to each tab button
struct TabData {
    SessionDocumentsBar* bar;
    fs::path path;
};
}  // namespace

SessionDocumentsBar::SessionDocumentsBar(Control* control):
        control(control), box(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2), xoj::util::adopt) {
    gtk_widget_set_name(box.get(), "sessionDocumentsBar");
    gtk_widget_set_margin_start(box.get(), 4);
    gtk_widget_set_margin_top(box.get(), 2);
    gtk_widget_set_margin_bottom(box.get(), 2);
    gtk_widget_set_no_show_all(box.get(), true);
    gtk_widget_hide(box.get());
}

SessionDocumentsBar::~SessionDocumentsBar() {
    if (rebuildSource != 0) {
        g_source_remove(rebuildSource);
    }
}

auto SessionDocumentsBar::getWidget() const -> GtkWidget* { return box.get(); }

void SessionDocumentsBar::setCurrentDocument(const fs::path& path) {
    bool changed = false;
    if (!path.empty() && std::find(documents.begin(), documents.end(), path) == documents.end()) {
        documents.push_back(path);
        changed = true;
    }
    if (path != current) {
        current = path;
        changed = true;
    }
    if (changed) {
        // May be called from within one of our button handlers (e.g. a synchronous save)
        scheduleRebuild();
    }
}

void SessionDocumentsBar::rebuild() {
    GtkWidget* container = box.get();

    GList* children = gtk_container_get_children(GTK_CONTAINER(container));
    for (GList* it = children; it; it = it->next) {
        gtk_widget_destroy(GTK_WIDGET(it->data));
    }
    g_list_free(children);

    // A single notebook needs no switcher
    if (documents.size() < 2) {
        gtk_widget_hide(container);
        return;
    }

    for (const fs::path& path: documents) {
        const bool isCurrent = path == current;

        GtkWidget* tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkStyleContext* tabStyle = gtk_widget_get_style_context(tab);
        gtk_style_context_add_class(tabStyle, "linked");

        std::string name{char_cast(path.stem().u8string())};
        GtkWidget* button = gtk_toggle_button_new_with_label(name.c_str());
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), isCurrent);
        gtk_widget_set_tooltip_text(button, char_cast(path.u8string().c_str()));
        if (isCurrent) {
            gtk_style_context_add_class(gtk_widget_get_style_context(button), "suggested-action");
        }
        g_signal_connect_data(button, "clicked",
                              G_CALLBACK(+[](GtkButton*, TabData* d) { d->bar->switchTo(d->path); }),
                              new TabData{this, path}, xoj::util::closure_notify_cb<TabData>, GConnectFlags(0));
        gtk_box_pack_start(GTK_BOX(tab), button, false, false, 0);

        if (!isCurrent) {
            GtkWidget* closeBtn = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
            gtk_widget_set_tooltip_text(closeBtn, _("Remove from this list"));
            g_signal_connect_data(closeBtn, "clicked",
                                  G_CALLBACK(+[](GtkButton*, TabData* d) { d->bar->remove(d->path); }),
                                  new TabData{this, path}, xoj::util::closure_notify_cb<TabData>, GConnectFlags(0));
            gtk_box_pack_start(GTK_BOX(tab), closeBtn, false, false, 0);
        }

        gtk_box_pack_start(GTK_BOX(container), tab, false, false, 0);
        gtk_widget_show_all(tab);
    }
    gtk_widget_show(container);
}

void SessionDocumentsBar::scheduleRebuild() {
    // Rebuilding destroys the clicked button (and the data of its handler): never do it during the emission
    if (rebuildSource == 0) {
        rebuildSource = g_idle_add(+[](gpointer self) -> gboolean {
            auto* bar = static_cast<SessionDocumentsBar*>(self);
            bar->rebuildSource = 0;
            bar->rebuild();
            return G_SOURCE_REMOVE;
        }, this);
    }
}

void SessionDocumentsBar::switchTo(fs::path path) {
    if (path == current) {
        scheduleRebuild();  // restore the toggle state the click changed
        return;
    }

    if (std::error_code err; !fs::exists(path, err)) {
        remove(path);
        return;
    }

    auto open = [ctrl = this->control, path]() { ctrl->openFile(path, [](bool) {}, -1, false); };

    // Save the notebook we are leaving without asking, so switching back and forth is painless.
    // Unsaved new documents (no file yet) still get the usual "Save / Discard / Cancel" question in openFile.
    if (control->getUndoRedoHandler()->isChanged() && !current.empty() && fs::exists(current)) {
        control->save([open](bool saved) {
            if (saved) {
                open();
            }
        });
    } else {
        open();
    }
    // Undo the toggle the click caused; the new document highlights itself once it is loaded
    scheduleRebuild();
}

void SessionDocumentsBar::remove(fs::path path) {
    if (path == current) {
        return;
    }
    documents.erase(std::remove(documents.begin(), documents.end(), path), documents.end());
    scheduleRebuild();
}
