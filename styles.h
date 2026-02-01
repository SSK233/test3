#ifndef STYLES_H
#define STYLES_H

#include <QString>

namespace Styles
{
    const QString BUTTON_SELECTED_STYLE = 
        "background-color: #707d98; "
        "color: #ebecef; "
        "font-weight: bold; "
        "font-size: 20px; "
        "border-radius: 5px; "
        "border: none;";

    const QString BUTTON_UNSELECTED_STYLE = 
        "background-color: #ebecef; "
        "color: #707d98; "
        "font-size: 20px; "
        "border-radius: 5px; "
        "border: none;";

    const QString WINDOW_BACKGROUND_STYLE = 
        "QMainWindow { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #a8edea, stop:1 #fed6e3); "
        "}";

    const QString CENTRAL_WIDGET_STYLE = 
        "QWidget { background: transparent; }";

    const QString PUSH_BUTTON_STYLE = 
        "QPushButton { "
        "background-color: #ebecef; "
        "color: #707d98; "
        "font-size: 20px; "
        "border-radius: 10px; "
        "border: none; "
        "} "
        "QPushButton:pressed { "
        "background-color: #707d98; "
        "color: #ebecef; "
        "}";

    const QString LINE_EDIT_STYLE = 
        "QLineEdit { "
        "background-color: #ffffff; "
        "color: #707d98; "
        "font-size: 14px; "
        "border: 1px solid #000000; "
        "border-radius: 5px; "
        "padding-left: 5px; "
        "}";

    const QString TOP_BAR_STYLE = 
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #74ebd5, stop:1 #9face6); "
        "border: none; "
        "}";

    const QString BLUR_TRANSITION_STYLE = 
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(116,235,213,0.8), stop:0.5 rgba(116,235,213,0.3), stop:1 rgba(26,44,79,0)); "
        "border: none; "
        "}";

    const QString SERIAL_BUTTON_STYLE = 
        "QPushButton { "
        "background-color: #ebecef; "
        "color: #707d98; "
        "border-radius: 5px; "
        "border: none; "
        "font-size: 14px; "
        "} "
        "QPushButton:pressed { "
        "background-color: #707d98; "
        "color: #ebecef; "
        "}";

    const QString COMBO_BOX_STYLE = 
        "QComboBox { "
        "background-color: #ebecef; "
        "color: #707d98; "
        "border-radius: 5px; "
        "border: 1px solid #707d98; "
        "padding: 5px; "
        "} "
        "QComboBox::drop-down { "
        "border: none; "
        "} "
        "QComboBox::down-arrow { "
        "image: none; "
        "} "
        "QListView { "
        "background-color: #ebecef; "
        "color: #707d98; "
        "border: 1px solid #707d98; "
        "border-radius: 5px; "
        "padding: 0; "
        "selection-background-color: #707d98; "
        "selection-color: #ebecef; "
        "outline: none; "
        "margin: 0; "
        "show-decoration-selected: 0; "
        "} "
        "QListView::item { "
        "padding: 8px 12px; "
        "margin: 0; "
        "border: none; "
        "} "
        "QListView::item:selected { "
        "background-color: #707d98; "
        "color: #ebecef; "
        "border-radius: 3px; "
        "} "
        "QComboBox QFrame { "
        "border-radius: 5px; "
        "}";
}

#endif // STYLES_H
