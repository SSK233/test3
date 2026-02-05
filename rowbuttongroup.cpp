/**
 * @file rowbuttongroup.cpp
 * @brief 行按钮组管理类实现文件
 * @details 包含RowButtonGroup类的实现，负责处理按钮点击事件、文本框输入事件，以及与Modbus寄存器的通信
 * @version 1.0
 * @date 2024
 */

#include "rowbuttongroup.h"
#include "mainwindow.h"
#include "modbusmanager.h"
#include "styles.h"
#include <QDebug>
#include <QTimer>
#include <QLocale>
#include <limits.h>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 * @details 初始化成员变量，创建编辑计时器并设置信号槽连接
 */
RowButtonGroup::RowButtonGroup(QObject *parent)
    : QObject(parent), lineEdit(nullptr), m_isUpdating(false), isEditing(false), isDebouncing(false), fanState(false), fanButton(nullptr)
{
    recentlyChangedRegisters.clear();
    
    editTimer = new QTimer(this);
    editTimer->setSingleShot(true);
    connect(editTimer, &QTimer::timeout, this, [this]() {
        isEditing = false;
        qDebug() << "行" << rowIndex << "编辑超时，恢复自动更新";
    });
    
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    connect(debounceTimer, &QTimer::timeout, this, [this]() {
        isDebouncing = false;
        qDebug() << "行" << rowIndex << "防抖超时，恢复按钮响应";
    });
}

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
 * @param lineEdit 显示总和的文本框
 * @param mainWindow 主窗口指针
 * @param rowIndex 行索引
 * @param address Modbus寄存器地址
 * @details 初始化按钮列表、对应数值、按钮状态，并连接信号槽
 */
void RowButtonGroup::initialize(QPushButton *btn1, QPushButton *btn2, QPushButton *btn4,
                               QPushButton *btn8, QPushButton *btn16, QPushButton *btn32,
                               QPushButton *btn64, QPushButton *loadBtn, QPushButton *unloadBtn, QPushButton *fanBtn, QLineEdit *lineEdit, MainWindow *mainWindow, int rowIndex, int address, int loadUnloadAddress)
{
    buttons = {btn1, btn2, btn4, btn8, btn16, btn32, btn64};

    values = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0};

    states.resize(buttons.size(), false);

    this->lineEdit = lineEdit;
    this->mainWindow = mainWindow;
    this->rowIndex = rowIndex;
    this->registerAddress = address;
    this->loadUnloadRegisterAddress = loadUnloadAddress;
    this->fanButton = fanBtn;

    for (int i = 0; i < buttons.size(); ++i) {
        connect(buttons[i], &QPushButton::clicked, this, &RowButtonGroup::onButtonClicked);
    }

    connect(loadBtn, &QPushButton::clicked, this, &RowButtonGroup::onLoadButtonClicked);

    connect(unloadBtn, &QPushButton::clicked, this, &RowButtonGroup::onUnloadButtonClicked);

    connect(fanBtn, &QPushButton::clicked, this, &RowButtonGroup::onFanButtonClicked);

    connect(lineEdit, &QLineEdit::textChanged, this, &RowButtonGroup::onLineEditTextChanged);

    connect(lineEdit, &QLineEdit::selectionChanged, this, [this, rowIndex]() {
        isEditing = true;
        editTimer->start(2000);
        qDebug() << "行" << rowIndex << "文本框被点击，编辑模式开启";
    });

    connect(lineEdit, &QLineEdit::cursorPositionChanged, this, [this, rowIndex]() {
        isEditing = true;
        editTimer->start(2000);
        qDebug() << "行" << rowIndex << "光标移动，重置编辑计时器";
    });

    applyButtonStatesToUI();
    updateFanButtonState();

    updateSumDisplay();
}

/**
 * @brief 按钮点击事件处理函数
 * @details 处理按钮点击事件，切换按钮状态，更新UI和显示，并将状态写入Modbus寄存器
 *          仅处理第一行按钮(rowIndex == 0)，使用寄存器高8位存储按钮状态
 */
void RowButtonGroup::onButtonClicked()
{
    if (m_isUpdating || isDebouncing) return;

    // 设置防抖状态，防止快速连续点击
    isDebouncing = true;
    debounceTimer->start(200); // 500ms防抖

    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    qDebug() << "按钮被点击 - objectName:" << button->objectName() << "rowIndex:" << rowIndex;

    int index = buttons.indexOf(button);
    qDebug() << "按钮索引:" << index << "按钮数量:" << buttons.size();
    
    if (index != -1 && index < BUTTON_COUNT) {
        states[index] = !states[index];
        applyButtonStatesToUI();
        updateSumDisplay();
        
        qDebug() << "按钮状态更新成功 - rowIndex:" << rowIndex << "registerAddress:" << registerAddress;
        
        if (rowIndex == 0) {
                qDebug() << "正在处理第一行按钮，准备写入寄存器" << registerAddress;
                
                mainWindow->pauseRefreshTimer();
                
                int registerValue = 0;
                for (int i = 0; i < BUTTON_COUNT; ++i) {
                    registerValue |= (states[i] ? 1 : 0) << i;
                }
                
                recentlyChangedRegisters.insert(registerAddress);
                
                qDebug() << "按钮状态编码完成，准备读取寄存器低8位 - registerValue:" << registerValue;
                
                if (!ModbusManager::instance()->isStable()) {
                    qDebug() << "Modbus连接尚未稳定，等待后再尝试操作";
                    recentlyChangedRegisters.remove(registerAddress);
                    mainWindow->resumeRefreshTimer();
                    return;
                }
                
                ModbusManager::instance()->writeRegister(registerAddress, registerValue & 0x00FF);
                
                recentlyChangedRegisters.remove(registerAddress);
                this->mainWindow->resumeRefreshTimer();
        } else {
            qDebug() << "行" << rowIndex << "的按钮点击暂未实现";
        }
    } else {
        qDebug() << "按钮索引无效或超出范围 - index:" << index;
    }
}

/**
 * @brief 更新总和显示
 * @details 计算所有选中按钮的数值总和，并更新到文本框中显示
 */
void RowButtonGroup::updateSumDisplay()
{
    if (!lineEdit) return;

    double sum = 0.0;
    for (int i = 0; i < states.size(); ++i) {
        if (states[i]) {
            sum += values[i];
        }
    }

    bool wasUpdating = m_isUpdating;
    
    m_isUpdating = true;
    
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    
    lineEdit->setText(locale.toString(sum, 'f', 1));
    
    m_isUpdating = wasUpdating;
}

/**
 * @brief 应用按钮状态到UI
 * @details 根据按钮状态设置按钮的样式，选中的按钮使用选中样式，未选中的按钮使用未选中样式
 */
void RowButtonGroup::applyButtonStatesToUI()
{
    for (int i = 0; i < buttons.size(); ++i) {
        if (states[i]) {
            buttons[i]->setStyleSheet(Styles::BUTTON_SELECTED_STYLE);
        } else {
            buttons[i]->setStyleSheet(Styles::BUTTON_UNSELECTED_STYLE);
        }
    }
}

/**
 * @brief 更新风机按钮状态
 * @details 根据风机状态更新按钮的样式和文本
 */
void RowButtonGroup::updateFanButtonState()
{
    if (!fanButton) return;
    
    if (fanState) {
        fanButton->setStyleSheet(Styles::SLIDER_BUTTON_ON_STYLE);
        fanButton->setText("开");
    } else {
        fanButton->setStyleSheet(Styles::SLIDER_BUTTON_OFF_STYLE);
        fanButton->setText("关");
    }
}

/**
 * @brief 文本框内容变化事件处理函数
 * @param text 文本框的新内容
 * @details 仅标记编辑状态，不自动同步到按钮状态
 */
void RowButtonGroup::onLineEditTextChanged(const QString &text)
{
    if (m_isUpdating) return;
    
    isEditing = true;
    editTimer->start(2000);
    qDebug() << "行" << rowIndex << "文本正在编辑，重置编辑计时器";
}

/**
 * @brief 载入按钮点击事件处理函数
 * @details 将文本框的值同步到按钮状态并写入Modbus寄存器
 */
void RowButtonGroup::onLoadButtonClicked()
{
    if (m_isUpdating || isDebouncing) return;

    // 设置防抖状态，防止快速连续点击
    isDebouncing = true;
    debounceTimer->start(200); // 500ms防抖

    if (rowIndex != 0) {
        qDebug() << "行" << rowIndex << "的载入暂未实现";
        return;
    }

    qDebug() << "点击载入按钮，准备设置寄存器0的第1位";
    
    mainWindow->pauseRefreshTimer();
    
    recentlyChangedRegisters.insert(loadUnloadRegisterAddress);
    
    // 读取当前寄存器值
    ModbusManager::instance()->readRegister(loadUnloadRegisterAddress, [this](int currentValue) {
        if (currentValue != -1) {
            qDebug() << "当前寄存器值:" << currentValue;
            
            // 将第1位置1
            int setValue = currentValue | (1 << 1);
            qDebug() << "设置后的值:" << setValue;
            ModbusManager::instance()->writeRegister(loadUnloadRegisterAddress, setValue & 0x00FF);
            
            // 100ms后将第1位置0
            QTimer::singleShot(100, this, [this, currentValue]() {
                int resetValue = currentValue & ~(1 << 1);
                qDebug() << "重置后的值:" << resetValue;
                ModbusManager::instance()->writeRegister(loadUnloadRegisterAddress, resetValue & 0x00FF);
                
                recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
                mainWindow->resumeRefreshTimer();
            });
        } else {
            qDebug() << "读取寄存器失败";
            recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
            mainWindow->resumeRefreshTimer();
        }
    });
}

/**
 * @brief 求解按钮组合
 * @param targetSum 目标总和
 * @details 使用递归回溯法求解最小组合，使得选中的按钮数值总和等于目标值
 */
void RowButtonGroup::solveButtonStates(double targetSum)
{
    QVector<int> intValues;
    for (double v : values) {
        intValues.append(static_cast<int>(v * 10));
    }

    int target = static_cast<int>(targetSum * 10);
    
    QVector<bool> bestUsed;
    int bestCount = INT_MAX;

    QVector<bool> used(values.size(), false);
    solveCombinations(target, intValues, 0, used, bestUsed, bestCount);

    if (bestCount != INT_MAX) {
        states = bestUsed;
    } else {
        states.fill(false);
    }
}

/**
 * @brief 递归求解组合
 * @param target 目标值(放大10倍的整数值)
 * @param values 按钮数值数组(放大10倍的整数值)
 * @param index 当前处理的索引
 * @param used 当前使用状态
 * @param bestUsed 最优解使用状态
 * @param bestCount 最优解按钮数量
 * @details 递归回溯算法，尝试所有可能的按钮组合，找到按钮数量最少的组合
 */
void RowButtonGroup::solveCombinations(int target, const QVector<int> &values, int index, QVector<bool> &used, QVector<bool> &bestUsed, int &bestCount)
{
    if (target == 0) {
        int currentCount = 0;
        for (bool b : used) {
            if (b) currentCount++;
        }
        
        if (currentCount < bestCount) {
            bestCount = currentCount;
            bestUsed = used;
        }
        return;
    }

    if (index >= values.size() || target < 0) {
        return;
    }

    used[index] = true;
    solveCombinations(target - values[index], values, index + 1, used, bestUsed, bestCount);
    used[index] = false;
    solveCombinations(target, values, index + 1, used, bestUsed, bestCount);
}

/**
 * @brief 卸载按钮点击事件处理函数
 * @details 点击卸载按钮时，将寄存器0的第2位置1，100ms后再置0
 */
void RowButtonGroup::onUnloadButtonClicked()
{
    if (m_isUpdating || isDebouncing) return;

    // 设置防抖状态，防止快速连续点击
    isDebouncing = true;
    debounceTimer->start(200); // 500ms防抖

    if (rowIndex != 0) {
        qDebug() << "行" << rowIndex << "的卸载暂未实现";
        return;
    }

    qDebug() << "点击卸载按钮，准备设置寄存器0的第2位";
    
    mainWindow->pauseRefreshTimer();
    
    recentlyChangedRegisters.insert(loadUnloadRegisterAddress);
    
    // 读取当前寄存器值
    ModbusManager::instance()->readRegister(loadUnloadRegisterAddress, [this](int currentValue) {
        if (currentValue != -1) {
            qDebug() << "当前寄存器值:" << currentValue;
            
            // 将第2位置1
            int setValue = currentValue | (1 << 2);
            qDebug() << "设置后的值:" << setValue;
            ModbusManager::instance()->writeRegister(loadUnloadRegisterAddress, setValue & 0x00FF);
            
            // 100ms后将第2位置0
            QTimer::singleShot(100, this, [this, currentValue]() {
                int resetValue = currentValue & ~(1 << 2);
                qDebug() << "重置后的值:" << resetValue;
                ModbusManager::instance()->writeRegister(loadUnloadRegisterAddress, resetValue & 0x00FF);
                
                recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
                mainWindow->resumeRefreshTimer();
            });
        } else {
            qDebug() << "读取寄存器失败";
            recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
            mainWindow->resumeRefreshTimer();
        }
    });
}

/**
 * @brief 风机开关按钮点击事件处理函数
 * @details 处理风机开关按钮点击事件，切换风机状态，更新UI，并将状态写入寄存器的第0位
 */
void RowButtonGroup::onFanButtonClicked()
{
    if (m_isUpdating || isDebouncing) return;

    // 设置防抖状态，防止快速连续点击
    isDebouncing = true;
    debounceTimer->start(200);

    qDebug() << "点击风机开关按钮，当前状态:" << fanState;
    
    // 切换风机状态
    fanState = !fanState;
    
    // 更新按钮UI
    updateFanButtonState();
    
    if (rowIndex == 0) {
        qDebug() << "正在处理第一行风机开关，准备写入寄存器" << loadUnloadRegisterAddress;
        
        mainWindow->pauseRefreshTimer();
        
        recentlyChangedRegisters.insert(loadUnloadRegisterAddress);
        
        if (!ModbusManager::instance()->isStable()) {
            qDebug() << "Modbus连接尚未稳定，等待后再尝试操作";
            recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
            mainWindow->resumeRefreshTimer();
            return;
        }
        
        // 读取当前寄存器值
        ModbusManager::instance()->readRegister(loadUnloadRegisterAddress, [this](int currentValue) {
            if (currentValue != -1) {
                qDebug() << "当前寄存器值:" << currentValue;
                
                int setValue;
                if (fanState) {
                    // 将第0位置1
                    setValue = currentValue | (1 << 0);
                } else {
                    // 将第0位置0
                    setValue = currentValue & ~(1 << 0);
                }
                
                qDebug() << "设置后的值:" << setValue;
                ModbusManager::instance()->writeRegister(loadUnloadRegisterAddress, setValue & 0x00FF);
                
                recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
                mainWindow->resumeRefreshTimer();
            } else {
                qDebug() << "读取寄存器失败";
                recentlyChangedRegisters.remove(loadUnloadRegisterAddress);
                mainWindow->resumeRefreshTimer();
            }
        });
    } else {
        qDebug() << "行" << rowIndex << "的风机开关暂未实现";
    }
}
