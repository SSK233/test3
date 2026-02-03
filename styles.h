/**
 * @file styles.h
 * @brief 样式定义文件
 * @details 包含应用程序中使用的各种UI组件样式常量，定义了按钮、窗口、文本框等的外观样式
 */

#ifndef STYLES_H
#define STYLES_H

#include <QString>

/**
 * @namespace Styles
 * @brief 样式命名空间
 * @details 包含应用程序中所有UI组件的样式定义
 */
namespace Styles
{
    /**
     * @brief 选中状态按钮样式
     * @details 定义按钮被选中时的外观，包括背景色、文字颜色、字体大小等
     */
    const QString BUTTON_SELECTED_STYLE = 
        "background-color: #707d98; "
        "color: #ebecef; "
        "font-weight: bold; "
        "font-size: 20px; "
        "border-radius: 5px; "
        "border: none;";

    /**
     * @brief 未选中状态按钮样式
     * @details 定义按钮未被选中时的外观，包括背景色、文字颜色、字体大小等
     */
    const QString BUTTON_UNSELECTED_STYLE = 
        "background-color: #ebecef; "
        "color: #707d98; "
        "font-size: 20px; "
        "border-radius: 5px; "
        "border: none;";

    /**
     * @brief 窗口背景样式
     * @details 定义主窗口的背景渐变效果
     */
    const QString WINDOW_BACKGROUND_STYLE = 
        "QMainWindow { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #a8edea, stop:1 #fed6e3); "
        "}";

    /**
     * @brief 中央部件样式
     * @details 定义中央部件的背景为透明
     */
    const QString CENTRAL_WIDGET_STYLE = 
        "QWidget { background: transparent; }";

    /**
     * @brief 通用按钮样式
     * @details 定义通用按钮的外观，包括普通状态和按下状态
     */
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

    /**
     * @brief 文本框样式
     * @details 定义文本框的外观，包括背景色、边框、字体大小等
     */
    const QString LINE_EDIT_STYLE = 
        "QLineEdit { "
        "background-color: #ffffff; "
        "color: #707d98; "
        "font-size: 14px; "
        "border: 1px solid #000000; "
        "border-radius: 5px; "
        "padding-left: 5px; "
        "}";

    /**
     * @brief 顶部栏样式
     * @details 定义顶部栏的背景渐变效果
     */
    const QString TOP_BAR_STYLE = 
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #74ebd5, stop:1 #9face6); "
        "border: none; "
        "}";

    /**
     * @brief 模糊过渡样式
     * @details 定义模糊过渡效果的背景渐变
     */
    const QString BLUR_TRANSITION_STYLE = 
        "QWidget { "
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(116,235,213,0.8), stop:0.5 rgba(116,235,213,0.3), stop:1 rgba(26,44,79,0)); "
        "border: none; "
        "}";

    /**
     * @brief 串口相关按钮样式
     * @details 定义串口相关按钮的外观，包括普通状态和按下状态
     */
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

    /**
     * @brief 下拉框样式
     * @details 定义下拉框的外观，包括主控件和下拉列表的样式
     */
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
