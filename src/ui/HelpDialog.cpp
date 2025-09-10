//  ----------------------------------
//  XMALab -- Copyright (c) 2015,
//  Brown University (GPL v3)
//  ----------------------------------
//
///\file HelpDialog.cpp
///\author Benjamin Knorlein
///\date 11/04/2016
///
/// Dynamically adapts shortcut key “caps” to the active theme.
/// Rules:
///   Theme == "dark" -> dark key caps
///   Theme == "light" -> white key caps
///   Theme == "system" -> follow current application palette lightness
/// White caps are kept ONLY when the effective theme is light.

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/HelpDialog.h"
#include "ui_HelpDialog.h"
#include "core/Settings.h"

#include <QApplication>
#include <QPalette>
#include <QLabel>
#include <QStyle>

using namespace xma;

namespace {

// Determine whether we should use the dark variant
bool effectiveDarkTheme() {
    QString theme = Settings::getInstance()->getQStringSetting("Theme");
    if (theme == "dark")
        return true;
    if (theme == "light")
        return false;
    // theme == "system" (or anything unexpected): infer from current palette
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

// Apply a unified stylesheet to all keycap labels.
// We treat labels that had the inline "background: white" (from the .ui file)
// as keycaps and remove their per-label style so they can be themed.
void restyleKeycaps(QWidget *root) {
    const bool dark = effectiveDarkTheme();

    // Consolidated styles
    static const QString lightStyle =
        "QLabel[keycap='true'] {"
        "  border:1px solid gray;"
        "  border-radius:5px;"
        "  padding:2px 6px;"
        "  background:#ffffff;"
        "  color:#000000;"
        "}"
        "QLabel[keycap='true']:disabled { color:#888888; }";

    static const QString darkStyle =
        "QLabel[keycap='true'] {"
        "  border:1px solid #5a5a5a;"
        "  border-radius:5px;"
        "  padding:2px 6px;"
        "  background:#2f2f2f;"
        "  color:#e6e6e6;"
        "}"
        "QLabel[keycap='true']:disabled { color:#777777; }";

    const auto labels = root->findChildren<QLabel*>();

    for (QLabel *lbl : labels) {
        const QString ss = lbl->styleSheet();
        // Original .ui sets: border:1px ... background: white;
        if (ss.contains("background", Qt::CaseInsensitive) &&
            ss.contains("white", Qt::CaseInsensitive)) {
            // Strip inline style so our consolidated style can take over
            lbl->setStyleSheet(QString());
            lbl->setProperty("keycap", true);
        } else if (!lbl->property("keycap").isValid()) {
            // Heuristic: very short text & no spaces -> likely a key label
            QString t = lbl->text().trimmed();
            if (!t.isEmpty() && t.size() <= 6 && !t.contains(' ') &&
                t.length() <= 10) {
                // Avoid long descriptive labels
                if (!(t.contains("frame", Qt::CaseInsensitive) ||
                      t.contains("track", Qt::CaseInsensitive)))
                    lbl->setProperty("keycap", true);
            }
        }
    }

    // Remove any previously injected block to keep idempotent
    QString base = root->styleSheet();
    if (base.contains("QLabel[keycap='true']"))
        base.clear();

    base += (dark ? darkStyle : lightStyle);
    root->setStyleSheet(base);

    // Re-polish so Qt reapplies palette + style
    for (QLabel *lbl : labels) {
        if (lbl->property("keycap").isValid()) {
            lbl->style()->unpolish(lbl);
            lbl->style()->polish(lbl);
            lbl->update();
        }
    }
}

} // namespace

HelpDialog::HelpDialog(QWidget* parent) :
    QDialog(parent),
    diag(new Ui::HelpDialog)
{
    diag->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Help"));

#ifdef __APPLE__
    // Replace "Ctrl" with Command symbol in shortcuts section only
    const auto macLabels = diag->groupBox_2->findChildren<QLabel*>();
    for (QLabel *lbl : macLabels) {
        if (lbl->text() == "Ctrl")
            lbl->setText(QChar(0x2318));
    }
#endif

    // Apply theme-aware styling of keycaps
    restyleKeycaps(this);
}

HelpDialog::~HelpDialog()
{
    delete diag;
}
