#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "RowButtonGroup.h"
#include "styles.h"
#include <limits.h>
#include <QDebug>
#include <QTimer>
#include <QListView>
#include <QEventLoop>

QSerialPort *COM = new QSerialPort();
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

    // 等待Modbus连接建立并稳定后再开始刷新
    // 不再使用提前刷新，而是通过refreshTimer定期刷新
    
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
    
    // 为按钮设置样式
    ui->btnVoltageWaveform->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    ui->btnBackToMain->setStyleSheet(Styles::SERIAL_BUTTON_STYLE);
    
    // 初始化波形图
    initVoltageWaveform();
    
    // 连接波形图按钮点击事件
    connect(ui->btnVoltageWaveform, &QPushButton::clicked, this, &MainWindow::switchToWaveformPage);
    connect(ui->btnBackToMain, &QPushButton::clicked, this, &MainWindow::switchToMainPage);
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
    
    // 设置超时时间（毫秒）- 增加到500ms以提高稳定性
    modbusMaster->setTimeout(400);
    
    // 设置重试次数 - 增加到2次重试
    modbusMaster->setNumberOfRetries(1);
    
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
        // 重置连接稳定标志
        MainWindow::m_modbusStable = false;
        // 延迟2秒后标记连接稳定，给设备足够的初始化时间
        QTimer::singleShot(500, this, []() {
            MainWindow::m_modbusStable = true;
            qDebug() << "Modbus连接已稳定，可以开始正常通信";
        });
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
            connect(reply, &QModbusReply::finished, modbusMaster, [reply, address, value]() {
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
    
    // 检查Modbus连接是否稳定
    if (!MainWindow::m_modbusStable) {
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
bool MainWindow::m_modbusStable = false;

void MainWindow::readSlave3Register7()
{
    if (!modbusMaster) {
        return;
    }
    
    if (modbusMaster->state() != QModbusDevice::ConnectedState) {
        return;
    }
    
    // 检查Modbus连接是否稳定
    if (!MainWindow::m_modbusStable) {
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
                
                // 更新波形图数据
                updateWaveformData(voltage);
                
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
        }
    }
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
void MainWindow::initVoltageWaveform()
{
    // 初始化波形图数据
    voltageData.reserve(MAX_DATA_POINTS);
    dataPointCount = 0;
    
    // 设置波形图图表
    setupWaveformChart();
    
    // 初始化波形图更新定时器
    waveformUpdateTimer = new QTimer(this);
    connect(waveformUpdateTimer, &QTimer::timeout, this, [this]() {
        // 读取电压数据并更新波形图
        readSlave3Register7();
    });
    
    // 设置定时器间隔为1000ms（1Hz更新频率）
    waveformUpdateTimer->setInterval(1000);
}

/**
 * @brief 设置波形图图表
 */
void MainWindow::setupWaveformChart()
{
    // 创建图表
    voltageChart = new QChart();
    voltageChart->setTitle("电压实时波形图");
    voltageChart->setAnimationOptions(QChart::NoAnimation);
    voltageChart->setMargins(QMargins(10, 10, 10, 30)); // 调整边距，确保X轴有足够空间显示
    voltageChart->legend()->setVisible(true);
    
    // 创建数据系列
    voltageSeries = new QLineSeries();
    voltageSeries->setName("电压 (V)");
    voltageChart->addSeries(voltageSeries);
    
    // 创建坐标轴
    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("时间 (s)");
    axisX->setRange(0, MAX_DATA_POINTS); // 500个点，每个点1秒
    voltageChart->addAxis(axisX, Qt::AlignBottom);
    voltageSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("电压 (V)");
    axisY->setRange(228, 233); // 电压范围228-232
    voltageChart->addAxis(axisY, Qt::AlignLeft);
    voltageSeries->attachAxis(axisY);
    
    // 创建图表视图
    chartView = new QChartView(voltageChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    // 调整图表视图的大小，确保有足够空间显示X轴
    QRect containerRect = ui->chartContainer->geometry();
    QRect chartRect = containerRect.adjusted(30, 30, -30, -80); // 进一步增加内边距，底部留出更多空间给X轴
    chartView->setGeometry(chartRect);
    chartView->setParent(ui->voltageWaveformPage);
    
    // 设置图表容器的样式
    ui->chartContainer->setStyleSheet("background-color: white;");
}

/**
 * @brief 切换到波形图页面
 */
void MainWindow::switchToWaveformPage()
{
    // 隐藏centralwidget中的所有子组件，保留centralwidget本身可见
    ui->centralwidget->findChild<QWidget*>("topBar")->setVisible(false);
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
    waveformUpdateTimer->start();
    
    qDebug() << "已切换到波形图页面";
}

/**
 * @brief 切换到主界面
 */
void MainWindow::switchToMainPage()
{
    // 停止波形图更新定时器
    waveformUpdateTimer->stop();
    
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
void MainWindow::updateWaveformData(double voltage)
{
    // 添加新数据点
    voltageData.append(voltage);
    dataPointCount++;
    
    // 限制数据点数量
    if (voltageData.size() > MAX_DATA_POINTS) {
        voltageData.removeFirst();
    }
    
    // 更新波形图
    voltageSeries->clear();
    for (int i = 0; i < voltageData.size(); i++) {
        voltageSeries->append(i, voltageData[i]); // X坐标是i秒
    }
    
    // 更新图表
    voltageChart->update();
}
