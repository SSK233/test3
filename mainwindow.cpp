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

    // 设置窗口最小尺寸为初始尺寸，确保只能放大不能缩小
    this->setMinimumSize(this->size());
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
    ui->topBar->setGeometry(0, 0, newWidth, ui->topBar->height());
    
    // 调整blurTransition宽度以适应窗口宽度
    ui->blurTransition->setGeometry(0, ui->blurTransition->y(), newWidth, ui->blurTransition->height());
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
                // 从寄存器的高8位提取按钮的状态
                row->m_isUpdating = true;
                for (int i = 0; i < BUTTON_COUNT; ++i) {
                    int bitPosition = 8 + i;
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
    
    // 读取寄存器的低8位（保持不变），将高8位置0
    ModbusManager::instance()->readRegister(row->registerAddress, [row](int lowValue) {
        if (lowValue != -1) {
            // 保留低8位，高8位置0
            int newValue = (lowValue & 0x00FF) | 0x0000;
            
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
    // 隐藏blurTransition，但保留topBar可见（因为raiseEffect在topBar中）
    ui->centralwidget->findChild<QWidget*>("blurTransition")->setVisible(false);
    
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
