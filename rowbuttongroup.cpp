/**
 * @file rowbuttongroup.cpp
 * @brief 行按钮组管理类实现文件
 * @details 包含RowButtonGroup类的实现，负责处理按钮点击事件、文本框输入事件，以及与Modbus寄存器的通信
 */

#include "rowbuttongroup.h"
#include "mainwindow.h"
#include "modbusmanager.h"
#include "styles.h"
#include <QDebug>
#include <QTimer>
#include <QLocale>
#include <limits.h>

RowButtonGroup::RowButtonGroup(QObject *parent)
    : QObject(parent), lineEdit(nullptr), m_isUpdating(false), isEditing(false)
{
    recentlyChangedRegisters.clear();
    
    editTimer = new QTimer(this);
    editTimer->setSingleShot(true);
    connect(editTimer, &QTimer::timeout, this, [this]() {
        isEditing = false;
        qDebug() << "行" << rowIndex << "编辑超时，恢复自动更新";
    });
}

void RowButtonGroup::initialize(QPushButton *btn0_1, QPushButton *btn0_2, QPushButton *btn0_2_2,
                               QPushButton *btn0_5, QPushButton *btn1, QPushButton *btn2,
                               QPushButton *btn2_2, QPushButton *btn5, QLineEdit *lineEdit, MainWindow *mainWindow, int rowIndex, int address)
{
    buttons = {btn0_1, btn0_2, btn0_2_2, btn0_5, btn1, btn2, btn2_2, btn5};

    values = {0.1, 0.2, 0.2, 0.5, 1.0, 2.0, 2.0, 5.0};

    states.resize(buttons.size(), false);

    this->lineEdit = lineEdit;
    this->mainWindow = mainWindow;
    this->rowIndex = rowIndex;
    this->registerAddress = address;

    for (int i = 0; i < buttons.size(); ++i) {
        connect(buttons[i], &QPushButton::clicked, this, &RowButtonGroup::onButtonClicked);
    }

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

    updateSumDisplay();
}

void RowButtonGroup::onButtonClicked()
{
    if (m_isUpdating) return;

    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;

    qDebug() << "按钮被点击 - objectName:" << button->objectName() << "rowIndex:" << rowIndex;

    int index = buttons.indexOf(button);
    qDebug() << "按钮索引:" << index << "按钮数量:" << buttons.size();
    
    if (index != -1 && index < 8) {
        states[index] = !states[index];
        applyButtonStatesToUI();
        updateSumDisplay();
        
        qDebug() << "按钮状态更新成功 - rowIndex:" << rowIndex << "registerAddress:" << registerAddress;
        
        if (rowIndex == 0) {
                qDebug() << "正在处理第一行按钮，准备写入寄存器" << registerAddress;
                
                mainWindow->pauseRefreshTimer();
                
                int registerValue = 0;
                for (int i = 0; i < 8; ++i) {
                    registerValue |= (states[i] ? 1 : 0) << (8 + i);
                }
                
                recentlyChangedRegisters.insert(registerAddress);
                
                qDebug() << "按钮状态编码完成，准备读取寄存器低8位 - registerValue:" << registerValue;
                
                if (!ModbusManager::instance()->isStable()) {
                    qDebug() << "Modbus连接尚未稳定，等待后再尝试操作";
                    recentlyChangedRegisters.remove(registerAddress);
                    mainWindow->resumeRefreshTimer();
                    return;
                }
                
                ModbusManager::instance()->readRegister(registerAddress, [this, registerValue](int lowValue) {
                    qDebug() << "读取寄存器回调 - lowValue:" << lowValue << "registerValue:" << registerValue;
                    if (lowValue != -1) {
                        int newValue = (lowValue & 0x00FF) | (registerValue & 0xFF00);
                        
                        qDebug() << "准备写入寄存器 - 地址:" << registerAddress << "新值:" << newValue;
                        
                        ModbusManager::instance()->writeRegister(registerAddress, newValue);
                        
                        recentlyChangedRegisters.remove(registerAddress);
                        qDebug() << "清理缓冲区 - registerAddress:" << registerAddress;
                        
                        this->mainWindow->resumeRefreshTimer();
                    } else {
                        qDebug() << "读取寄存器失败，lowValue为-1";
                        recentlyChangedRegisters.remove(registerAddress);
                        this->mainWindow->resumeRefreshTimer();
                    }
                });
        } else {
            qDebug() << "行" << rowIndex << "的按钮点击暂未实现";
        }
    } else {
        qDebug() << "按钮索引无效或超出范围 - index:" << index;
    }
}

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

void RowButtonGroup::onLineEditTextChanged(const QString &text)
{
    if (m_isUpdating) return;
    
    isEditing = true;
    editTimer->start(2000);
    qDebug() << "行" << rowIndex << "文本正在编辑，重置编辑计时器";

    if (m_isUpdating || !lineEdit) return;

    if (rowIndex != 0) {
        qDebug() << "行" << rowIndex << "的文本编辑暂未实现";
        return;
    }

    QLocale locale;
    bool ok;
    double sum = locale.toDouble(text, &ok);

    if (ok && sum >= 0.0 && sum <= 10.0) {
        m_isUpdating = true;
        
        solveButtonStates(sum);
        
        applyButtonStatesToUI();
        
        m_isUpdating = false;
        
        int registerValue = 0;
        for (int i = 0; i < 8; ++i) {
            registerValue |= (states[i] ? 1 : 0) << (8 + i);
        }
        
        recentlyChangedRegisters.insert(registerAddress);
        
        ModbusManager::instance()->readRegister(registerAddress, [this, registerValue](int lowValue) {
            if (lowValue != -1) {
                int newValue = (lowValue & 0x00FF) | (registerValue & 0xFF00);
                
                ModbusManager::instance()->writeRegister(registerAddress, newValue);
            }
        });
        
        QTimer::singleShot(2000, this, [this]() {
            recentlyChangedRegisters.clear();
        });
    } 
    else if (text.isEmpty()) {
        m_isUpdating = true;
        
        states.fill(false);
        
        applyButtonStatesToUI();
        
        m_isUpdating = false;
        
        recentlyChangedRegisters.insert(registerAddress);
        
        ModbusManager::instance()->readRegister(registerAddress, [this](int lowValue) {
            if (lowValue != -1) {
                int newValue = (lowValue & 0x00FF) | 0x0000;
                
                ModbusManager::instance()->writeRegister(registerAddress, newValue);
            }
        });
        
        QTimer::singleShot(2000, this, [this]() {
            recentlyChangedRegisters.clear();
        });
    }
}

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
