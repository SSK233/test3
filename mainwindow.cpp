#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "styles.h"
#include <limits.h>
#include <QDebug>
#include <QTimer>
#include <QListView>
#include <QEventLoop>

QSerialPort *COM = new QSerialPort();
bool MainWindow::m_serialPortOpen = false;
/**
 * @brief RowButtonGroup类构造函数
 * @param parent 父对象指针
 * @details 初始化成员变量，设置初始状态
 */
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

/**
 * @brief 初始化RowButtonGroup
 * @param btn0_1     0.1按钮指针
 * @param btn0_2     0.2按钮指针（第一个）
 * @param btn0_2_2   0.2按钮指针（第二个）
 * @param btn0_5     0.5按钮指针
 * @param btn1       1按钮指针
 * @param btn2       2按钮指针（第一个）
 * @param btn2_2     2按钮指针（第二个）
 * @param btn5       5按钮指针
 * @param lineEdit   输入框指针
 * @param mainWindow 主窗口指针
 * @details 建立按钮与数值的映射关系，连接信号槽，初始化显示
 */
void RowButtonGroup::initialize(QPushButton *btn0_1, QPushButton *btn0_2, QPushButton *btn0_2_2,
                               QPushButton *btn0_5, QPushButton *btn1, QPushButton *btn2,
                               QPushButton *btn2_2, QPushButton *btn5, QLineEdit *lineEdit, MainWindow *mainWindow, int rowIndex, int address)
{
    // 将按钮按顺序存入向量
    buttons = {btn0_1, btn0_2, btn0_2_2, btn0_5, btn1, btn2, btn2_2, btn5};

    // 为每个按钮设置对应的数值：0.1, 0.2, 0.2, 0.5, 1.0, 2.0, 2.0, 5.0
    // 这组数值能够精确组合出0到10之间的任何精确到小数点后1位的数字
    values = {0.1, 0.2, 0.2, 0.5, 1.0, 2.0, 2.0, 5.0};

    // 初始化所有按钮状态为未选中（false）
    states.resize(buttons.size(), false);

    // 保存输入框与主窗口指针
    this->lineEdit = lineEdit;
    this->mainWindow = mainWindow;
    this->rowIndex = rowIndex;
    this->registerAddress = address;

    // 连接所有按钮的点击事件到onButtonClicked槽函数
    for (int i = 0; i < buttons.size(); ++i) {
        connect(buttons[i], &QPushButton::clicked, this, &RowButtonGroup::onButtonClicked);
    }

    // 连接输入框文本变化事件到onLineEditTextChanged槽函数
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

    // 初始更新按钮样式（未选中状态）
    applyButtonStatesToUI();

    // 初始更新总和显示为0.0
    updateSumDisplay();
}

/**
 * @brief 按钮点击事件处理函数
 * @details 切换被点击按钮的选中状态，更新UI显示和总和
 */
void RowButtonGroup::onButtonClicked()
{
    // 如果正在更新状态，直接返回，避免递归调用
    if (m_isUpdating) return;

    // 获取发送信号的按钮对象
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;  // 如果无法转换类型，返回

    // 查找被点击按钮在buttons向量中的索引
    int index = buttons.indexOf(button);
    if (index != -1 && index < 8) {  // 确保只处理前8个按钮
        states[index] = !states[index];  // 切换按钮状态（选中/未选中）
        applyButtonStatesToUI();        // 更新按钮UI样式
        updateSumDisplay();            // 更新总和显示
        
        // 第一行：8个按钮控制寄存器1的高8位
        if (rowIndex == 0) {
            // 将8个按钮状态编码为一个16位值的高8位
            int registerValue = 0;
            for (int i = 0; i < 8; ++i) {
                registerValue |= (states[i] ? 1 : 0) << (8 + i);  // 按钮0对应第8位，按钮7对应第15位
            }
            
            // 读取寄存器的低8位（保持不变）
            recentlyChangedRegisters.insert(registerAddress);
            
            mainWindow->readRegister(registerAddress, [this, registerValue](int lowValue) {
                if (lowValue != -1) {
                    // 保留低8位，只更新高8位
                    int newValue = (lowValue & 0x00FF) | (registerValue & 0xFF00);
                    
                    MainWindow::writeRegister(registerAddress, newValue);
                    
                    // 延迟清理缓冲区（2秒后），避免长时间影响后续读取
                    QTimer::singleShot(2000, this, [this]() {
                        recentlyChangedRegisters.remove(registerAddress);
                    });
                }
            });
        } else {
            // 其他行：暂时不实现
            qDebug() << "行" << rowIndex << "的按钮点击暂未实现";
        }
    }
}

/**
 * @brief 更新输入框中显示的总和
 * @details 计算所有选中按钮的数值总和，显示在输入框中
 */
void RowButtonGroup::updateSumDisplay()
{
    // 安全检查：若输入框未初始化则直接返回
    if (!lineEdit) return;

    // 遍历所有按钮，累加选中按钮对应的数值
    double sum = 0.0;
    for (int i = 0; i < states.size(); ++i) {
        if (states[i]) {  // 如果按钮被选中
            sum += values[i];  // 累加其数值
        }
    }

    // 暂存当前更新状态，避免覆盖原有设置
    bool wasUpdating = m_isUpdating;
    
    // 开启更新锁，避免触发输入框文本变化的递归回调
    m_isUpdating = true;
    
    // 初始化区域设置，禁用千位分隔符以保证数字格式统一
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    
    // 将计算结果格式化为1位小数的字符串并显示在输入框
    lineEdit->setText(locale.toString(sum, 'f', 1));
    
    // 恢复之前的更新锁状态
    m_isUpdating = wasUpdating;
}

/**
 * @brief 根据按钮状态更新UI显示
 * @details 为选中的按钮设置背景色，未选中的按钮恢复默认样式
 */
void RowButtonGroup::applyButtonStatesToUI()
{
    // 遍历所有按钮，根据状态应用不同样式
    for (int i = 0; i < buttons.size(); ++i) {
        if (states[i]) {
            buttons[i]->setStyleSheet(Styles::BUTTON_SELECTED_STYLE);
        } else {
            buttons[i]->setStyleSheet(Styles::BUTTON_UNSELECTED_STYLE);
        }
    }
}

/**
 * @brief 输入框文本变化事件处理函数
 * @param text 输入框中的文本
 * @details 根据输入的数值，自动选择相应的按钮使其总和等于输入值
 */
void RowButtonGroup::onLineEditTextChanged(const QString &text)
{
    if (m_isUpdating) return;
    
    isEditing = true;
    editTimer->start(2000);
    qDebug() << "行" << rowIndex << "文本正在编辑，重置编辑计时器";

    // 如果正在更新状态或输入框不存在，直接返回
    if (m_isUpdating || !lineEdit) return;

    // 只处理第一行
    if (rowIndex != 0) {
        qDebug() << "行" << rowIndex << "的文本编辑暂未实现";
        return;
    }

    // 将文本转换为浮点数
    QLocale locale;
    bool ok;
    double sum = locale.toDouble(text, &ok);

    // 如果转换成功且数值在0到10之间
    if (ok && sum >= 0.0 && sum <= 10.0) {
        // 设置更新标志，防止递归触发
        m_isUpdating = true;
        
        // 求解按钮状态，使选中按钮的总和等于输入值
        solveButtonStates(sum);
        
        // 更新按钮UI样式
        applyButtonStatesToUI();
        
        // 清除更新标志
        m_isUpdating = false;
        
        // 将8个按钮状态编码为一个16位值的高8位
        int registerValue = 0;
        for (int i = 0; i < 8; ++i) {
            registerValue |= (states[i] ? 1 : 0) << (8 + i);  // 按钮0对应第8位，按钮7对应第15位
        }
        
        // 读取寄存器的低8位（保持不变）
        recentlyChangedRegisters.insert(registerAddress);
        
        mainWindow->readRegister(registerAddress, [this, registerValue](int lowValue) {
            if (lowValue != -1) {
                // 保留低8位，只更新高8位
                int newValue = (lowValue & 0x00FF) | (registerValue & 0xFF00);
                
                MainWindow::writeRegister(registerAddress, newValue);
            }
        });
        
        // 延迟清理缓冲区（2秒后），避免长时间影响后续读取
        QTimer::singleShot(2000, this, [this]() {
            recentlyChangedRegisters.clear();
        });
    } 
    // 如果文本为空，清除所有按钮状态
    else if (text.isEmpty()) {
        // 设置更新标志，防止递归触发
        m_isUpdating = true;
        
        // 清除所有按钮状态
        states.fill(false);
        
        // 更新按钮UI样式
        applyButtonStatesToUI();
        
        // 清除更新标志
        m_isUpdating = false;
        
        // 读取寄存器的低8位（保持不变），将高8位置0
        recentlyChangedRegisters.insert(registerAddress);
        
        mainWindow->readRegister(registerAddress, [this](int lowValue) {
            if (lowValue != -1) {
                // 保留低8位，高8位置0
                int newValue = (lowValue & 0x00FF) | 0x0000;
                
                MainWindow::writeRegister(registerAddress, newValue);
            }
        });
        
        // 延迟清理缓冲区（2秒后），避免长时间影响后续读取
        QTimer::singleShot(2000, this, [this]() {
            recentlyChangedRegisters.clear();
        });
    }
}

/**
 * @brief 求解按钮状态：根据目标总和设置相应按钮亮起
 * @param targetSum 目标总和（精确到小数点后1位）
 * @details 为避免浮点数精度误差，先将所有数值乘以10转换为整数运算。
 *          调用递归回溯算法寻找最优按钮组合，确保使用最少的按钮达到目标总和，
 *          最终将求解结果同步到states向量中以更新UI状态。
 */
void RowButtonGroup::solveButtonStates(double targetSum)
{
    // 将浮点数值转换为整数，避免浮点精度问题
    QVector<int> intValues;
    for (double v : values) {
        intValues.append(static_cast<int>(v * 10));
    }

    // 将目标和也转换为整数
    int target = static_cast<int>(targetSum * 10);
    
    // 初始化最优解变量
    QVector<bool> bestUsed;
    int bestCount = INT_MAX;

    // 初始化使用标记向量，用于递归回溯
    QVector<bool> used(values.size(), false);
    // 调用递归函数求解最优组合
    solveCombinations(target, intValues, 0, used, bestUsed, bestCount);

    // 如果找到有效解，则更新按钮状态
    if (bestCount != INT_MAX) {
        states = bestUsed;
    } else {
        // 未找到解时，清空所有按钮状态
        states.fill(false);
    }
}

/**
 * @brief 递归求解组合问题，找到和为target的最少元素组合
 * @param target 目标和
 * @param values 可用数值列表
 * @param index 当前处理的元素索引
 * @param used 标记元素是否被使用
 * @param bestUsed 记录最优解的使用情况
 * @param bestCount 记录最优解的元素数量
 * @details 使用递归回溯算法，尝试所有可能的组合，找到和为target的最少元素组合
 */
void RowButtonGroup::solveCombinations(int target, const QVector<int> &values, int index, QVector<bool> &used, QVector<bool> &bestUsed, int &bestCount)
{
    // 找到一个有效组合
    if (target == 0) {
        // 计算当前组合使用的元素数量
        int currentCount = 0;
        for (bool b : used) {
            if (b) currentCount++;
        }
        
        // 如果当前组合使用的元素更少，则更新最优解
        if (currentCount < bestCount) {
            bestCount = currentCount;
            bestUsed = used;
        }
        return;
    }

    // 递归终止条件：处理完所有元素或目标和为负数
    if (index >= values.size() || target < 0) {
        return;
    }

    // 选择当前元素
    used[index] = true;
    solveCombinations(target - values[index], values, index + 1, used, bestUsed, bestCount);
    // 回溯：不选择当前元素
    used[index] = false;
    solveCombinations(target, values, index + 1, used, bestUsed, bestCount);
}

/**
 * @brief MainWindow类构造函数
 * @param parent 父对象指针
 * @details 初始化窗口，创建UI，初始化所有行的按钮组
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // 设置UI
    ui->setupUi(this);

    // 设置窗口背景为对角渐变
    this->setStyleSheet(Styles::WINDOW_BACKGROUND_STYLE);
    ui->centralwidget->setStyleSheet(Styles::CENTRAL_WIDGET_STYLE);

    // 初始化Modbus RTU通信（在串口打开后初始化）
    // initModbus(); // 移到串口打开时初始化

    // 初始化定时刷新定时器
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshAllRows);
    refreshTimer->start(1000);  // 每1秒刷新一次

    // 初始化从站3第7个寄存器读取定时器
    slave3Timer = new QTimer(this);
    connect(slave3Timer, &QTimer::timeout, this, &MainWindow::readSlave3Register7);
    slave3Timer->start(1000);  // 每1秒读取一次

    // 等待Modbus连接建立后立即刷新一次
    QTimer::singleShot(100, this, &MainWindow::refreshAllRows);
    
    // 初始化串口状态
    MainWindow::m_serialPortOpen = false;
    
    // 初始化串口下拉框为启用状态
    ui->comboBox_available_COM->setEnabled(true);

    // 初始化第0行（第一行）
    row0.initialize(ui->btn_0_1, ui->btn_0_2, ui->btn_0_2_2, ui->btn_0_5, ui->btn_1, ui->btn_2, ui->btn_2_2, ui->btn_5, ui->lineEditSum, this, 0, REGISTER_ADDRESS_ROW0);

    // 初始化第1行
    row1.initialize(ui->btn1_0_1, ui->btn1_0_2, ui->btn1_0_2_2, ui->btn1_0_5, ui->btn1_1, ui->btn1_2, ui->btn1_2_2, ui->btn1_5, ui->lineEditSum1, this, 1, REGISTER_ADDRESS_ROW1);

    // 初始化第2行
    row2.initialize(ui->btn2_0_1, ui->btn2_0_2, ui->btn2_0_2_2, ui->btn2_0_5, ui->btn2_1, ui->btn2_2, ui->btn2_2_2, ui->btn2_5, ui->lineEditSum2, this, 2, REGISTER_ADDRESS_ROW2);

    // 初始化第3行
    row3.initialize(ui->btn3_0_1, ui->btn3_0_2, ui->btn3_0_2_2, ui->btn3_0_5, ui->btn3_1, ui->btn3_2, ui->btn3_2_2, ui->btn3_5, ui->lineEditSum3, this, 3, REGISTER_ADDRESS_ROW3);

    // 初始化第4行
    row4.initialize(ui->btn4_0_1, ui->btn4_0_2, ui->btn4_0_2_2, ui->btn4_0_5, ui->btn4_1, ui->btn4_2, ui->btn4_2_2, ui->btn4_5, ui->lineEditSum4, this, 4, REGISTER_ADDRESS_ROW4);

    // 初始化第5行
    row5.initialize(ui->btn5_0_1, ui->btn5_0_2, ui->btn5_0_2_2, ui->btn5_0_5, ui->btn5_1, ui->btn5_2, ui->btn5_2_2, ui->btn5_5, ui->lineEditSum5, this, 5, REGISTER_ADDRESS_ROW5);

    // 初始化第6行
    row6.initialize(ui->btn6_0_1, ui->btn6_0_2, ui->btn6_0_2_2, ui->btn6_0_5, ui->btn6_1, ui->btn6_2, ui->btn6_2_2, ui->btn6_5, ui->lineEditSum6, this, 6, REGISTER_ADDRESS_ROW6);

    // 初始化第7行
    row7.initialize(ui->btn7_0_1, ui->btn7_0_2, ui->btn7_0_2_2, ui->btn7_0_5, ui->btn7_1, ui->btn7_2, ui->btn7_2_2, ui->btn7_5, ui->lineEditSum7, this, 7, REGISTER_ADDRESS_ROW7);

    // 初始化第8行（第九行）
    row8.initialize(ui->btn8_0_1, ui->btn8_0_2, ui->btn8_0_2_2, ui->btn8_0_5, ui->btn8_1, ui->btn8_2, ui->btn8_2_2, ui->btn8_5, ui->lineEditSum8, this, 8, REGISTER_ADDRESS_ROW8);

    // 为所有"载入"和"卸载"按钮应用样式
    QList<QPushButton*> pushButtons = this->findChildren<QPushButton*>();
    for (QPushButton* btn : pushButtons) {
        if (btn->objectName().startsWith("pushButton")) {
            btn->setStyleSheet(Styles::PUSH_BUTTON_STYLE);
        }
    }
    
    // 为所有文本框应用样式（白色背景，黑色边框，圆角）
    QList<QLineEdit*> lineEdits = this->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        edit->setStyleSheet(Styles::LINE_EDIT_STYLE);
    }
    
    // 为串口管理控件应用样式
    ui->key_Refresh_COM->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    ui->key_OpenOrClose_COM->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    ui->comboBox_available_COM->setStyleSheet(Styles::COMBO_BOX_STYLE);
    
    // 为textBrowser去掉边框
    ui->textBrowser->setStyleSheet("border: none;");
    
    // 为顶部横条应用渐变样式
    ui->topBar->setStyleSheet(Styles::TOP_BAR_STYLE);
    
    // 为模糊过渡条应用渐变样式
    ui->blurTransition->setStyleSheet(Styles::BLUR_TRANSITION_STYLE);
    
    // 为下拉框设置视图，支持圆角效果
    QListView *view = new QListView(ui->comboBox_available_COM);
    view->setStyleSheet(Styles::COMBO_BOX_STYLE);
    ui->comboBox_available_COM->setView(view);
    
    // 连接卸载按钮点击事件
    connect(ui->pushButton_2, &QPushButton::clicked, this, [this]() { clearRow(0); });
    connect(ui->pushButton_11, &QPushButton::clicked, this, [this]() { clearRow(1); });
    connect(ui->pushButton_12, &QPushButton::clicked, this, [this]() { clearRow(2); });
    connect(ui->pushButton_13, &QPushButton::clicked, this, [this]() { clearRow(3); });
    connect(ui->pushButton_14, &QPushButton::clicked, this, [this]() { clearRow(4); });
    connect(ui->pushButton_15, &QPushButton::clicked, this, [this]() { clearRow(5); });
    connect(ui->pushButton_16, &QPushButton::clicked, this, [this]() { clearRow(6); });
    connect(ui->pushButton_17, &QPushButton::clicked, this, [this]() { clearRow(7); });
    connect(ui->pushButton_18, &QPushButton::clicked, this, [this]() { clearRow(8); });

    // 连接textBrowser文本变化事件
    connect(ui->textBrowser, &QTextBrowser::textChanged, this, &MainWindow::on_textBrowser_textChanged);

    // 初始化时隐藏raiseEffect组件
    ui->raiseEffect->hide();
}

/**
 * @brief 处理textBrowser文本变化事件
 * @details 当textBrowser有文字时显示raiseEffect组件，没有文字时隐藏
 */
void MainWindow::on_textBrowser_textChanged()
{
    QString text = ui->textBrowser->toPlainText().trimmed();
    
    if (text.isEmpty()) {
        ui->raiseEffect->hide();
    } else {
        ui->raiseEffect->show();
    }
}

/**
 * @brief 初始化Modbus RTU串行主站通信
 * @details 1. 创建QModbusRtuSerialMaster实例作为Modbus主站
 *          2. 配置标准Modbus RTU串口参数：COM5端口、9600波特率、8位数据位、无校验位、1位停止位
 *          3. 尝试建立与Modbus从设备的连接
 *          4. 输出连接结果日志信息便于调试
 */
void MainWindow::initModbus()
{
    // 如果Modbus已经连接，先断开
    if (modbusMaster) {
        if (modbusMaster->state() == QModbusDevice::ConnectedState) {
            modbusMaster->disconnectDevice();
        }
        delete modbusMaster;
        modbusMaster = nullptr;
    }
    
    // 检查串口是否已配置
    if (!COM || COM->portName().isEmpty()) {
        qDebug() << "Modbus初始化失败: 串口未配置";
        return;
    }
    
    // 如果全局串口已打开，先关闭它
    if (COM->isOpen()) {
        qDebug() << "关闭全局串口以让Modbus直接访问";
        COM->close();
    }
    
    // 创建Modbus主站实例
    modbusMaster = new QModbusRtuSerialMaster();
    
    // 配置Modbus连接参数
    modbusMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, COM->portName());
    modbusMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, COM->baudRate());
    modbusMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, COM->dataBits());
    modbusMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, COM->parity());
    modbusMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, COM->stopBits());
    
    // 设置超时时间（毫秒）
    modbusMaster->setTimeout(1000);
    
    // 设置重试次数
    modbusMaster->setNumberOfRetries(2);
    
    // 建立Modbus连接
    if (!modbusMaster->connectDevice()) {
        qDebug() << "Modbus连接失败:" << modbusMaster->errorString();
        delete modbusMaster;
        modbusMaster = nullptr;
        return;
    }
    
    // 等待连接建立
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);  // 等待500ms
    loop.exec();
    
    if (modbusMaster->state() == QModbusDevice::ConnectedState) {
        qDebug() << "Modbus连接成功 - 端口:" << COM->portName() << ", 波特率:" << COM->baudRate();
    } else {
        qDebug() << "Modbus连接超时 - 最终状态:" << modbusMaster->state();
        delete modbusMaster;
        modbusMaster = nullptr;
    }
}

/**
 * @brief 写入寄存器数据
 * @param address 寄存器地址
 * @param value   要写入的值
 */
void MainWindow::writeRegister(int address, int value)
{
    // 检查Modbus连接状态
    if (!modbusMaster) {
        qDebug() << "写入失败: Modbus主站未初始化";
        return;
    }
    
    if (modbusMaster->state() != QModbusDevice::ConnectedState) {
        qDebug() << "写入失败: Modbus未连接 - 当前状态:" << modbusMaster->state();
        return;
    }
    
    // 验证寄存器地址范围（0-65535）
    if (address < 0 || address > 65535) {
        qDebug() << "写入失败: 寄存器地址" << address << "超出范围(0-65535)";
        return;
    }
    
    qDebug() << "尝试写入寄存器 - 地址:" << address << "值:" << value;
    
    // 创建写寄存器请求单元
    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    writeUnit.setValue(0, value);
    
    // 发送写请求并处理响应
    if (auto *reply = modbusMaster->sendWriteRequest(writeUnit, 1)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, reply, [reply, address, value]() {
                if (reply->error() != QModbusDevice::NoError) {
                    qDebug() << "写入失败 - 地址:" << address << "值:" << value 
                             << "错误:" << reply->errorString() 
                             << "错误代码:" << reply->error();
                    
                    // 详细输出Modbus异常代码
                    if (reply->error() == QModbusDevice::ProtocolError) {
                        auto modbusReply = qobject_cast<QModbusReply*>(reply);
                        if (modbusReply && modbusReply->rawResult().isException()) {
                            int exceptionCode = modbusReply->rawResult().exceptionCode();
                            qDebug() << "Modbus异常代码:" << exceptionCode;
                            switch (exceptionCode) {
                                case 1: qDebug() << "异常说明: ILLEGAL FUNCTION (不支持的功能码)"; break;
                                case 2: qDebug() << "异常说明: ILLEGAL DATA ADDRESS (无效的寄存器地址)"; break;
                                case 3: qDebug() << "异常说明: ILLEGAL DATA VALUE (无效的寄存器值)"; break;
                                case 4: qDebug() << "异常说明: SERVER DEVICE FAILURE (设备故障)"; break;
                                case 5: qDebug() << "异常说明: ACKNOWLEDGE (确认，但需要时间)"; break;
                                case 6: qDebug() << "异常说明: SERVER DEVICE BUSY (设备忙)"; break;
                                case 7: qDebug() << "异常说明: MEMORY PARITY ERROR (内存校验错误)"; break;
                                case 8: qDebug() << "异常说明: GATEWAY PATH UNAVAILABLE (网关路径不可用)"; break;
                                case 9: qDebug() << "异常说明: GATEWAY TARGET FAILED (网关目标失败)"; break;
                                default: qDebug() << "异常说明: 未知异常"; break;
                            }
                        }
                    }
                } else {
                    qDebug() << "写入成功 - 地址:" << address << "值:" << value;
                }
                reply->deleteLater();
            });
        } else {
            qDebug() << "写入失败 - 地址:" << address << "值:" << value << "请求立即完成但无响应";
            reply->deleteLater();
        }
    } else {
        qDebug() << "写入请求发送失败 - 地址:" << address << "值:" << value 
                 << "错误:" << modbusMaster->errorString();
    }
}

/**
 * @brief 读取寄存器数据
 * @param address 寄存器地址
 * @param callback 读取完成后的回调函数
 */
void MainWindow::readRegister(int address, std::function<void(int)> callback)
{
    // 检查Modbus连接状态
    if (!modbusMaster) {
        qDebug() << "读取失败: Modbus主站未初始化";
        callback(-1);
        return;
    }
    
    if (modbusMaster->state() != QModbusDevice::ConnectedState) {
        qDebug() << "读取失败: Modbus未连接 - 当前状态:" << modbusMaster->state();
        callback(-1);
        return;
    }
    
    // 验证寄存器地址范围（0-65535）
    if (address < 0 || address > 65535) {
        qDebug() << "读取失败: 寄存器地址" << address << "超出范围(0-65535)";
        callback(-1);
        return;
    }
    
    qDebug() << "尝试读取寄存器 - 地址:" << address;
    
    // 创建读寄存器请求单元
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    
    // 发送读请求并处理响应
    if (auto *reply = modbusMaster->sendReadRequest(readUnit, 1)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [reply, address, callback]() {
                if (reply->error() != QModbusDevice::NoError) {
                    qDebug() << "读取失败 - 地址:" << address 
                             << "错误:" << reply->errorString() 
                             << "错误代码:" << reply->error();
                    
                    // 详细输出Modbus异常代码
                    if (reply->error() == QModbusDevice::ProtocolError) {
                        auto modbusReply = qobject_cast<QModbusReply*>(reply);
                        if (modbusReply && modbusReply->rawResult().isException()) {
                            int exceptionCode = modbusReply->rawResult().exceptionCode();
                            qDebug() << "Modbus异常代码:" << exceptionCode;
                            switch (exceptionCode) {
                                case 1: qDebug() << "异常说明: ILLEGAL FUNCTION (不支持的功能码)"; break;
                                case 2: qDebug() << "异常说明: ILLEGAL DATA ADDRESS (无效的寄存器地址)"; break;
                                case 3: qDebug() << "异常说明: ILLEGAL DATA VALUE (无效的寄存器值)"; break;
                                case 4: qDebug() << "异常说明: SERVER DEVICE FAILURE (设备故障)"; break;
                                case 5: qDebug() << "异常说明: ACKNOWLEDGE (确认，但需要时间)"; break;
                                case 6: qDebug() << "异常说明: SERVER DEVICE BUSY (设备忙)"; break;
                                case 7: qDebug() << "异常说明: MEMORY PARITY ERROR (内存校验错误)"; break;
                                case 8: qDebug() << "异常说明: GATEWAY PATH UNAVAILABLE (网关路径不可用)"; break;
                                case 9: qDebug() << "异常说明: GATEWAY TARGET FAILED (网关目标失败)"; break;
                                default: qDebug() << "异常说明: 未知异常"; break;
                            }
                        }
                    }
                    callback(-1);
                } else {
                    QModbusDataUnit result = reply->result();
                    int value = result.value(0);
                    qDebug() << "读取成功 - 地址:" << address << "值:" << value;
                    callback(value);
                }
                reply->deleteLater();
            });
        } else {
            qDebug() << "读取失败 - 地址:" << address << "请求立即完成但无响应";
            reply->deleteLater();
            callback(-1);
        }
    } else {
        qDebug() << "读取请求发送失败 - 地址:" << address 
                 << "错误:" << modbusMaster->errorString();
        callback(-1);
    }
}

/**
 * @brief 刷新所有行的状态
 */
void MainWindow::refreshAllRows()
{
    // 检查Modbus连接状态，避免在未连接时频繁输出错误
    if (!modbusMaster || modbusMaster->state() != QModbusDevice::ConnectedState) {
        // 每5秒只输出一次连接状态提示，避免刷屏
        static QTimer *connectionTimer = nullptr;
        static bool showed = false;
        
        if (!connectionTimer) {
            connectionTimer = new QTimer();
            connectionTimer->setSingleShot(true);
            connect(connectionTimer, &QTimer::timeout, []() {
                showed = false;
            });
        }
        
        if (!showed) {
            qDebug() << "等待Modbus连接...";
            showed = true;
            connectionTimer->start(5000);
        }
        return;
    }
    
    refreshRow(0);
}

/**
 * @brief 刷新指定行的状态
 * @param rowIndex 行索引（0-8）
 */
void MainWindow::refreshRow(int rowIndex)
{
    if (rowIndex != 0) return;
    
    RowButtonGroup *row = &row0;
    
    if (row->isEditing) {
        qDebug() << "行" << rowIndex << "正在编辑中，跳过自动更新";
        return;
    }
    
    // 读取寄存器（包含8个按钮的高8位状态）
    readRegister(row->registerAddress, [row](int value) {
        if (value != -1) {
            int registerAddress = row->registerAddress;
            
            if (row->isEditing) {
                qDebug() << "行正在编辑中，跳过寄存器" << registerAddress << "的更新";
                return;
            }
            
            if (row->recentlyChangedRegisters.contains(registerAddress)) {
                qDebug() << "寄存器" << registerAddress << "在缓冲区中，保留本地状态";
            } else {
                // 从寄存器的高8位提取8个按钮的状态
                row->m_isUpdating = true;
                for (int i = 0; i < 8; ++i) {
                    int bitPosition = 8 + i;  // 按钮0对应第8位，按钮7对应第15位
                    row->states[i] = (value >> bitPosition) & 0x0001;
                }
                row->applyButtonStatesToUI();
                row->updateSumDisplay();
                row->m_isUpdating = false;
                qDebug() << "寄存器" << registerAddress << "不在缓冲区中，使用Modbus值:" << value;
            }
        }
    });
}

/**
 * @brief 清除指定行的按钮状态和文本框内容
 * @param rowIndex 行索引（0-8）
 */
void MainWindow::clearRow(int rowIndex)
{
    // 只处理第一行（行0）
    if (rowIndex != 0) return;
    
    // 获取第一行的RowButtonGroup实例
    RowButtonGroup *row = &row0;
    
    // 清除所有按钮选中状态
    for (int i = 0; i < 8; ++i) {
        row->states[i] = false;
    }
    
    // 更新按钮UI样式为未选中状态
    row->applyButtonStatesToUI();
    
    // 清空文本框内容，重置为初始值0.0
    row->lineEdit->setText("0.0");
    
    // 将寄存器加入状态变更缓冲区
    row->recentlyChangedRegisters.insert(row->registerAddress);
    
    // 读取寄存器的低8位（保持不变），将高8位置0
    readRegister(row->registerAddress, [row](int lowValue) {
        if (lowValue != -1) {
            // 保留低8位，高8位置0
            int newValue = (lowValue & 0x00FF) | 0x0000;
            
            writeRegister(row->registerAddress, newValue);
        }
    });
    
    // 延迟清理缓冲区（2秒后），避免长时间影响后续读取
    QTimer::singleShot(2000, row, [row]() {
        row->recentlyChangedRegisters.clear();
    });
}


/**
 * @brief MainWindow类析构函数
 * @details 清理UI指针
 */
MainWindow::~MainWindow()
{
    delete ui;
}

// 初始化静态成员变量

/**
 * @brief 刷新串口按钮点击事件处理函数
 * @details 扫描系统中所有可用的串口，并将其添加到下拉框中
 */
void MainWindow::on_key_Refresh_COM_clicked()
{
    // 清空下拉框
    ui->comboBox_available_COM->clear();
    
    // 获取所有可用的串口信息
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    
    // 将串口名称添加到下拉框
    foreach (const QSerialPortInfo &portInfo, ports) {
        ui->comboBox_available_COM->addItem(portInfo.portName());
    }
    
    // 输出调试信息
    qDebug() << "刷新串口 - 找到" << ports.size() << "个可用串口";
}

/**
 * @brief 启动/关闭串口按钮点击事件处理函数
 * @details 根据当前状态，启动或关闭串口
 */
void MainWindow::on_key_OpenOrClose_COM_clicked()
{
    if (MainWindow::m_serialPortOpen) {
        // 当前串口已打开，执行关闭操作
        
        // 关闭串口
        if (COM->isOpen()) {
            COM->close();
        }
        
        // 关闭Modbus连接
        if (modbusMaster && modbusMaster->state() == QModbusDevice::ConnectedState) {
            modbusMaster->disconnectDevice();
        }
        
        // 更新状态标志
        MainWindow::m_serialPortOpen = false;
        
        // 更新radioButton状态
        ui->radioButton_checkOpen->setChecked(false);
        
        // 启用串口下拉框选择
        ui->comboBox_available_COM->setEnabled(true);
        
        // 更新按钮文字
        ui->key_OpenOrClose_COM->setText("启动串口");
        
        // 输出调试信息
        qDebug() << "串口已关闭";
    } else {
        // 当前串口已关闭，执行打开操作
        
        // 获取选中的端口名称
        QString portName = ui->comboBox_available_COM->currentText();
        if (portName.isEmpty()) {
            qDebug() << "未选择串口，请先刷新串口列表";
            return;
        }
        
        // 配置串口参数（只配置，不打开）
        COM->setPortName(portName);
        COM->setBaudRate(QSerialPort::Baud9600);          // 波特率9600
        COM->setDataBits(QSerialPort::Data8);              // 数据位8
        COM->setStopBits(QSerialPort::OneStop);            // 停止位1
        COM->setParity(QSerialPort::NoParity);             // 无校验位
        
        // 重新初始化Modbus（Modbus会自己打开串口）
        if (modbusMaster && modbusMaster->state() == QModbusDevice::ConnectedState) {
            modbusMaster->disconnectDevice();
            delete modbusMaster;
        }
        initModbus();
        
        // 更新状态标志
        MainWindow::m_serialPortOpen = true;
        
        // 更新radioButton状态
        ui->radioButton_checkOpen->setChecked(true);
        
        // 禁用串口下拉框选择
        ui->comboBox_available_COM->setEnabled(false);
        
        // 更新按钮文字
        ui->key_OpenOrClose_COM->setText("关闭串口");
        
        // 输出调试信息
        qDebug() << "Modbus初始化 - 端口:" << portName << "波特率: 9600";
    }
}

QModbusRtuSerialMaster *MainWindow::modbusMaster = nullptr;

void MainWindow::readSlave3Register7()
{
    if (!modbusMaster) {
        return;
    }
    
    if (modbusMaster->state() != QModbusDevice::ConnectedState) {
        return;
    }
    
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, 7, 1);
    
    if (auto *reply = modbusMaster->sendReadRequest(readUnit, 3)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [reply, this]() {
                if (reply->error() != QModbusDevice::NoError) {
                    reply->deleteLater();
                    return;
                }
                
                QModbusDataUnit result = reply->result();
                int value = result.value(0);
                
                double voltage = value * 0.1;
                
                QString displayStr = QString("电压: %1 V").arg(voltage, 0, 'f', 1);
                ui->textBrowser->setText(displayStr);
                
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    }
}