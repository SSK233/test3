/**
 * @file rowbuttongroup.h
 * @brief 行按钮组管理类定义文件
 * @details 包含RowButtonGroup类的声明，负责管理一行中的多个按钮状态、文本框输入和寄存器通信
 */

#ifndef ROWBUTTONGROUP_H
#define ROWBUTTONGROUP_H

#include <QObject>
#include <QPushButton>
#include <QLineEdit>
#include <QVector>
#include <QTimer>

const int BUTTON_COUNT = 7;

class MainWindow;

/**
 * @class RowButtonGroup
 * @brief 行按钮组管理类
 * @details 负责管理一行中的多个按钮状态、处理按钮点击事件、文本框输入事件，以及与Modbus寄存器的通信
 */
class RowButtonGroup : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    RowButtonGroup(QObject *parent = nullptr);

    /**
     * @brief 初始化行按钮组
     * @param btn1 1按钮
     * @param btn2 2按钮
     * @param btn4 4按钮
     * @param btn8 8按钮
     * @param btn16 16按钮
     * @param btn32 32按钮
     * @param btn64 64按钮
     * @param loadBtn 载入按钮
     * @param unloadBtn 卸载按钮
     * @param fanBtn 风机开关按钮
     * @param lineEdit 文本框
     * @param mainWindow 主窗口指针
     * @param rowIndex 行索引
     * @param address 按钮状态的寄存器地址
     * @param loadUnloadAddress 载入和卸载按钮的寄存器地址
     */
    void initialize(QPushButton *btn1, QPushButton *btn2, QPushButton *btn4,
                   QPushButton *btn8, QPushButton *btn16, QPushButton *btn32,
                   QPushButton *btn64, QPushButton *loadBtn, QPushButton *unloadBtn, QPushButton *fanBtn, QLineEdit *lineEdit, MainWindow *mainWindow, int rowIndex, int address, int loadUnloadAddress);

public:
    QVector<bool> states;                   // 按钮状态数组
    QLineEdit *lineEdit;                    // 文本框指针
    QSet<int> recentlyChangedRegisters;     // 跟踪最近修改的寄存器地址
    int registerAddress;                    // 按钮状态的寄存器地址
    int loadUnloadRegisterAddress;          // 载入和卸载按钮的寄存器地址
    bool fanState;                          // 风机开关状态
    
    /**
     * @brief 将按钮状态应用到UI
     */
    void applyButtonStatesToUI();
    
    /**
     * @brief 更新风机按钮状态
     * @details 根据风机状态更新按钮的样式和文本
     */
    void updateFanButtonState(); 

private slots:
    /**
     * @brief 按钮点击事件处理函数
     */
    void onButtonClicked();
    
    /**
     * @brief 文本框内容变化事件处理函数
     * @param text 新的文本内容
     */
    void onLineEditTextChanged(const QString &text);
    
    /**
     * @brief 载入按钮点击事件处理函数
     */
    void onLoadButtonClicked();
    
    /**
     * @brief 卸载按钮点击事件处理函数
     */
    void onUnloadButtonClicked();
    
    /**
     * @brief 风机开关按钮点击事件处理函数
     */
    void onFanButtonClicked();

private:
    QVector<QPushButton*> buttons;          // 按钮数组
    QVector<double> values;                 // 按钮对应的值数组
    MainWindow *mainWindow;                 // 主窗口指针
    QTimer *debounceTimer;                  // 按钮防抖定时器
    bool isDebouncing;                      // 是否处于防抖状态
    QPushButton *fanButton;                 // 风机开关按钮指针

    /**
     * @brief 根据目标和值求解按钮状态
     * @param targetSum 目标和值
     */
    void solveButtonStates(double targetSum);
    
    /**
     * @brief 求解组合问题
     * @param target 目标值
     * @param values 可用值数组
     * @param index 当前索引
     * @param used 当前使用状态
     * @param bestUsed 最佳使用状态
     * @param bestCount 最佳使用数量
     */
    void solveCombinations(int target, const QVector<int> &values, int index, QVector<bool> &used, QVector<bool> &bestUsed, int &bestCount);

public:
    bool m_isUpdating;                      // 是否正在更新
    int rowIndex;                           // 行索引
    bool m_sumLocked;                       // 和值显示是否锁定
    
    /**
     * @brief 更新和值显示
     */
    void updateSumDisplay();
    
    QTimer *editTimer;                      // 编辑定时器
    bool isEditing;                         // 是否正在编辑
};

#endif // ROWBUTTONGROUP_H
