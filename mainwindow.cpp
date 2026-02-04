/**
 * @file mainwindow.cpp
 * @brief 主窗口类实现文件
 * @details 包含MainWindow类的实现，负责应用程序的界面交互、串口管理、波形图显示等功能
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rowbuttongroup.h"
#include "styles.h"
#include "modbusmanager.h"
#include "waveformchart.h"
#include <limits.h>
#include <QDebug>
#include <QTimer>
#include <QListView>

bool MainWindow::m_serialPortOpen = false;

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

    // Modbus通信由ModbusManager管理

    // 初始化定时刷新定时器
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshAllRows);
    refreshTimer->start(1000);  // 每1秒刷新一次

    // 初始化从站3第7个寄存器读取定时器
    slave3Timer = new QTimer(this);
    connect(slave3Timer, &QTimer::timeout, this, &MainWindow::readSlave3Register7);
    slave3Timer->start(1000);  // 每1秒读取一次

    // 等待Modbus连接建立并稳定后再开始刷新
    // 不再使用提前刷新，而是通过refreshTimer定期刷新
    
    // 初始化串口状态
    MainWindow::m_serialPortOpen = false;
    
    // 初始化串口下拉框为启用状态
    ui->comboBox_available_COM->setEnabled(true);

    // 初始化第0行（第一行）
    row0.initialize(ui->btn_1, ui->btn_2, ui->btn_4, ui->btn_8, ui->btn_16, ui->btn_32, ui->btn_64, ui->pushButton_load, ui->lineEditSum, this, 0, REGISTER_ADDRESS_ROW0);



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
    ui->radioButton_checkOpen->setStyleSheet(Styles::RADIO_BUTTON_STYLE);
    
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

    // 连接textBrowser文本变化事件
    connect(ui->textBrowser, &QTextBrowser::textChanged, this, &MainWindow::on_textBrowser_textChanged);

    // 初始化时隐藏raiseEffect组件
    ui->raiseEffect->hide();
    
    // 为按钮设置样式
    ui->btnVoltageWaveform->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    ui->btnBackToMain->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    
    // 初始化波形图
    m_waveformChart = new WaveformChart(this);
    m_waveformChart->initVoltageWaveform(ui->chartContainer, ui->voltageWaveformPage);
    
    // 连接波形图按钮点击事件
    connect(ui->btnVoltageWaveform, &QPushButton::clicked, this, &MainWindow::switchToWaveformPage);
    connect(ui->btnBackToMain, &QPushButton::clicked, this, &MainWindow::switchToMainPage);

    // 设置窗口最小尺寸为能显示所有按钮的最小尺寸
    // 基于UI布局计算，确保所有控件都能正常显示
    this->setMinimumSize(1000, 600);
}

/**
 * @brief 处理窗口大小变化事件
 * @param event 大小变化事件
 * @details 当窗口大小变化时，调整topBar和blurTransition的宽度以适应窗口宽度
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 调整topBar宽度以适应窗口宽度
    int newWidth = this->width();
    int newHeight = this->height();
    ui->topBar->setGeometry(0, 0, newWidth, ui->topBar->height());
    
    // 调整blurTransition宽度以适应窗口宽度
    ui->blurTransition->setGeometry(0, ui->blurTransition->y(), newWidth, ui->blurTransition->height());
    
    // 调整topBar上一层控件的位置，保持距离右边框的距离固定
    // 基于初始布局计算固定的右边距（初始窗口宽度为1159）
    int fixedRightMargin_comboBox = 1159 - 880 - 131; // 131是comboBox的宽度
    int fixedRightMargin_keyRefresh = 1159 - 770 - 93; // 93是key_Refresh_COM的宽度
    int fixedRightMargin_keyOpenClose = 1159 - 1050 - 93; // 93是key_OpenOrClose_COM的宽度
    int fixedRightMargin_radioButton = 1159 - 1030 - 21; // 21是radioButton_checkOpen的宽度
    int fixedRightMargin_label = 1159 - 1030 - 25; // 25是label_19的宽度
    int fixedRightMargin_btnVoltage = 1159 - 600 - 120; // 120是btnVoltageWaveform的宽度
    
    // 重新计算控件的x坐标，保持距离右边框的距离固定
    int newX_comboBox = newWidth - fixedRightMargin_comboBox - 131;
    int newX_keyRefresh = newWidth - fixedRightMargin_keyRefresh - 93;
    int newX_keyOpenClose = newWidth - fixedRightMargin_keyOpenClose - 93;
    int newX_radioButton = newWidth - fixedRightMargin_radioButton - 21;
    int newX_label = newWidth - fixedRightMargin_label - 25;
    int newX_btnVoltage = newWidth - fixedRightMargin_btnVoltage - 120;
    
    // 应用新的位置
    ui->comboBox_available_COM->setGeometry(newX_comboBox, ui->comboBox_available_COM->y(), ui->comboBox_available_COM->width(), ui->comboBox_available_COM->height());
    ui->key_Refresh_COM->setGeometry(newX_keyRefresh, ui->key_Refresh_COM->y(), ui->key_Refresh_COM->width(), ui->key_Refresh_COM->height());
    ui->key_OpenOrClose_COM->setGeometry(newX_keyOpenClose, ui->key_OpenOrClose_COM->y(), ui->key_OpenOrClose_COM->width(), ui->key_OpenOrClose_COM->height());
    ui->radioButton_checkOpen->setGeometry(newX_radioButton, ui->radioButton_checkOpen->y(), ui->radioButton_checkOpen->width(), ui->radioButton_checkOpen->height());
    ui->label_19->setGeometry(newX_label, ui->label_19->y(), ui->label_19->width(), ui->label_19->height());
    ui->btnVoltageWaveform->setGeometry(newX_btnVoltage, ui->btnVoltageWaveform->y(), ui->btnVoltageWaveform->width(), ui->btnVoltageWaveform->height());
    
    // 调整波形图页面大小以适应窗口大小
    ui->voltageWaveformPage->setGeometry(0, 0, newWidth, newHeight);
    
    // 调整btnBackToMain按钮的位置，保持距离右边框的距离固定
    int fixedRightMargin_btnBackToMain = 1159 - 1000 - 120; // 120是btnBackToMain的宽度
    int newX_btnBackToMain = newWidth - fixedRightMargin_btnBackToMain - 120;
    ui->btnBackToMain->setGeometry(newX_btnBackToMain, ui->btnBackToMain->y(), ui->btnBackToMain->width(), ui->btnBackToMain->height());
    
    // 调整图表容器大小，使其随界面大小变化而变化并居中
    // 计算图表容器的新大小，距离blurTransition有10像素距离
    int chartContainerTop = ui->blurTransition->y() + ui->blurTransition->height() + 10;
    int chartContainerHeight = newHeight - chartContainerTop - 40; // 底部留40像素边距
    int chartContainerWidth = newWidth - 40; // 左右各留20像素边距
    
    // 确保图表容器大小合理
    if (chartContainerWidth < 200) chartContainerWidth = 200;
    if (chartContainerHeight < 200) chartContainerHeight = 200;
    
    // 计算图表容器的x坐标，使其水平居中
    int chartContainerX = (newWidth - chartContainerWidth) / 2;
    
    // 应用图表容器的新位置和大小
    QWidget *chartContainer = ui->voltageWaveformPage->findChild<QWidget*>("chartContainer");
    if (chartContainer) {
        chartContainer->setGeometry(chartContainerX, chartContainerTop, chartContainerWidth, chartContainerHeight);
        
        // 更新图表大小，使其与容器大小保持同步
        if (m_waveformChart) {
            m_waveformChart->updateChartSize(chartContainer);
        }
    }
    
    // 调整中间按钮组的位置，使它们整体居中
    // 增大按钮尺寸为50x50
    int buttonWidth = 50;
    int buttonHeight = 50;
    int buttonSpacing = 15;
    
    // 第一行：数字按钮组 (btn_1, btn_2, btn_4, btn_8, btn_16, btn_32, btn_64)
    int firstRowButtonCount = 7;
    int firstRowWidth = (buttonWidth * firstRowButtonCount) + (buttonSpacing * (firstRowButtonCount - 1));
    int firstRowStartX = (newWidth - firstRowWidth) / 2;
    int firstRowY = 325; // 上移一点，留出空间给下一行
    
    // 调整第一行数字按钮的位置和大小
    ui->btn_1->setGeometry(firstRowStartX, firstRowY, buttonWidth, buttonHeight);
    ui->btn_2->setGeometry(firstRowStartX + buttonWidth + buttonSpacing, firstRowY, buttonWidth, buttonHeight);
    ui->btn_4->setGeometry(firstRowStartX + (buttonWidth + buttonSpacing) * 2, firstRowY, buttonWidth, buttonHeight);
    ui->btn_8->setGeometry(firstRowStartX + (buttonWidth + buttonSpacing) * 3, firstRowY, buttonWidth, buttonHeight);
    ui->btn_16->setGeometry(firstRowStartX + (buttonWidth + buttonSpacing) * 4, firstRowY, buttonWidth, buttonHeight);
    ui->btn_32->setGeometry(firstRowStartX + (buttonWidth + buttonSpacing) * 5, firstRowY, buttonWidth, buttonHeight);
    ui->btn_64->setGeometry(firstRowStartX + (buttonWidth + buttonSpacing) * 6, firstRowY, buttonWidth, buttonHeight);
    
    // 第二行：输入框和操作按钮 (lineEditSum, label_10, pushButton_load, pushButton_2)
    int secondRowY = firstRowY + buttonHeight + 20; // 下移20像素
    int secondRowElementWidths[] = {80, 40, 80, 80}; // lineEditSum, label_10, pushButton_load, pushButton_2
    int secondRowSpacing = 15;
    int secondRowWidth = 80 + 40 + 80 + 80 + (secondRowSpacing * 3);
    int secondRowStartX = (newWidth - secondRowWidth) / 2;
    
    // 调整第二行元素的位置
    ui->lineEditSum->setGeometry(secondRowStartX, secondRowY, 80, buttonHeight);
    ui->label_10->setGeometry(secondRowStartX + 80 + secondRowSpacing, secondRowY + 5, 40, buttonHeight - 10);
    ui->pushButton_load->setGeometry(secondRowStartX + 80 + 40 + (secondRowSpacing * 2), secondRowY, 80, buttonHeight);
    ui->pushButton_2->setGeometry(secondRowStartX + 80 + 40 + 80 + (secondRowSpacing * 3), secondRowY, 80, buttonHeight);
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
 * @brief 刷新所有行的状态
 */
void MainWindow::refreshAllRows()
{
    // 检查Modbus连接状态，避免在未连接时频繁输出错误
    if (!ModbusManager::instance()->isConnected()) {
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
    
    // 检查Modbus连接是否稳定
    if (!ModbusManager::instance()->isStable()) {
        // 每3秒只输出一次连接稳定提示，避免刷屏
        static QTimer *stableTimer = nullptr;
        static bool stableShowed = false;
        
        if (!stableTimer) {
            stableTimer = new QTimer();
            stableTimer->setSingleShot(true);
            connect(stableTimer, &QTimer::timeout, []() {
                stableShowed = false;
            });
        }
        
        if (!stableShowed) {
            qDebug() << "Modbus连接已建立，等待设备稳定...";
            stableShowed = true;
            stableTimer->start(3000);
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
    ModbusManager::instance()->readRegister(row->registerAddress, [row](int value) {
        if (value != -1) {
            int registerAddress = row->registerAddress;
            
            if (row->isEditing) {
                qDebug() << "行正在编辑中，跳过寄存器" << registerAddress << "的更新";
                return;
            }
            
            if (row->recentlyChangedRegisters.contains(registerAddress)) {
                qDebug() << "寄存器" << registerAddress << "在缓冲区中，保留本地状态";
            } else {
                // 从寄存器的低8位提取按钮的状态
                row->m_isUpdating = true;
                for (int i = 0; i < BUTTON_COUNT; ++i) {
                    int bitPosition = i;
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
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        row->states[i] = false;
    }
    
    // 更新按钮UI样式为未选中状态
    row->applyButtonStatesToUI();
    
    // 清空文本框内容，重置为初始值0.0
    row->lineEdit->setText("0.0");
    
    // 将寄存器加入状态变更缓冲区
    row->recentlyChangedRegisters.insert(row->registerAddress);
    
    // 读取寄存器的高8位（保持不变），将低8位置0
    ModbusManager::instance()->readRegister(row->registerAddress, [row](int value) {
        if (value != -1) {
            // 保留高8位，低8位置0
            int newValue = (value & 0xFF00) | 0x0000;
            
            ModbusManager::instance()->writeRegister(row->registerAddress, newValue);
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
        
        // 关闭Modbus连接
        ModbusManager::instance()->closeModbus();
        
        // 清除textBrowser显示的电压信息
        ui->textBrowser->clear();
        
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
        
        // 初始化Modbus
        bool success = ModbusManager::instance()->initModbus(portName, 9600);
        if (success) {
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
        } else {
            qDebug() << "Modbus初始化失败";
        }
    }
}



void MainWindow::readSlave3Register7()
{
    ModbusManager::instance()->readSlave3Register7([this](int value) {
        if (value != -1) {
            double voltage = value * 0.1;
            
            QString displayStr = QString("电压: %1 V").arg(voltage, 0, 'f', 1);
            ui->textBrowser->setText(displayStr);
            
            // 更新波形图数据
            m_waveformChart->updateWaveformData(voltage);
        }
    });
}

/**
 * @brief 暂停定时刷新定时器
 * @details 用于在写入操作前暂停自动刷新，避免竞争条件
 */
void MainWindow::pauseRefreshTimer()
{
    if (refreshTimer && refreshTimer->isActive()) {
        refreshTimer->stop();
        qDebug() << "定时刷新已暂停";
    }
}

/**
 * @brief 恢复定时刷新定时器
 * @details 用于在写入操作后恢复自动刷新
 */
void MainWindow::resumeRefreshTimer()
{
    if (refreshTimer && !refreshTimer->isActive()) {
        refreshTimer->start(1000);
        qDebug() << "定时刷新已恢复";
    }
}

/**
 * @brief 初始化电压波形图
 */
/**
 * @brief 切换到波形图页面
 */
void MainWindow::switchToWaveformPage()
{
    // 保留blurTransition可见
    ui->centralwidget->findChild<QWidget*>("blurTransition")->setVisible(true);
    
    // 隐藏所有按钮，除了返回主界面的按钮
    QList<QPushButton*> buttons = ui->centralwidget->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        if (btn != ui->btnBackToMain) {
            btn->setVisible(false);
        }
    }
    
    QList<QLineEdit*> lineEdits = ui->centralwidget->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        edit->setVisible(false);
    }
    
    QList<QLabel*> labels = ui->centralwidget->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        label->setVisible(false);
    }
    
    QList<QComboBox*> comboboxes = ui->centralwidget->findChildren<QComboBox*>();
    for (QComboBox* box : comboboxes) {
        box->setVisible(false);
    }
    
    QList<QRadioButton*> radioButtons = ui->centralwidget->findChildren<QRadioButton*>();
    for (QRadioButton* radio : radioButtons) {
        radio->setVisible(false);
    }
    
    // 显示波形图页面
    ui->voltageWaveformPage->setVisible(true);
    
    // 启动波形图更新定时器
    m_waveformChart->startWaveformUpdate();
    
    qDebug() << "已切换到波形图页面";
}

/**
 * @brief 切换到主界面
 */
void MainWindow::switchToMainPage()
{
    // 停止波形图更新定时器
    m_waveformChart->stopWaveformUpdate();
    
    // 隐藏波形图页面
    ui->voltageWaveformPage->setVisible(false);
    
    // 显示centralwidget中的所有子组件
    ui->centralwidget->findChild<QWidget*>("topBar")->setVisible(true);
    ui->centralwidget->findChild<QWidget*>("blurTransition")->setVisible(true);
    
    // 显示所有按钮和输入框
    QList<QPushButton*> buttons = ui->centralwidget->findChildren<QPushButton*>();
    for (QPushButton* btn : buttons) {
        btn->setVisible(true);
    }
    
    QList<QLineEdit*> lineEdits = ui->centralwidget->findChildren<QLineEdit*>();
    for (QLineEdit* edit : lineEdits) {
        edit->setVisible(true);
    }
    
    QList<QLabel*> labels = ui->centralwidget->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        label->setVisible(true);
    }
    
    QList<QComboBox*> comboboxes = ui->centralwidget->findChildren<QComboBox*>();
    for (QComboBox* box : comboboxes) {
        box->setVisible(true);
    }
    
    QList<QRadioButton*> radioButtons = ui->centralwidget->findChildren<QRadioButton*>();
    for (QRadioButton* radio : radioButtons) {
        radio->setVisible(true);
    }
    
    // 确保textBrowser可见
    ui->textBrowser->setVisible(true);
    
    qDebug() << "已切换到主界面";
}

/**
 * @brief 更新波形图数据
 * @param voltage 电压值
 */
