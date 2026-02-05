/**
 * @file modbusmanager.cpp
 * @brief Modbus通信管理类实现文件
 * @details 包含ModbusManager类的实现，负责处理Modbus RTU串行通信的具体操作
 */

#include "modbusmanager.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QVariant>

// 初始化静态单例实例
ModbusManager* ModbusManager::m_instance = nullptr;

/**
 * @brief ModbusManager构造函数
 * @param parent 父对象指针
 */
ModbusManager::ModbusManager(QObject *parent) : QObject(parent)
{
    modbusMaster = nullptr;
    COM = new QSerialPort();
    m_serialPortOpen = false;
    m_modbusStable = false;
}

/**
 * @brief ModbusManager析构函数
 */
ModbusManager::~ModbusManager()
{
    closeModbus();
    delete COM;
}

/**
 * @brief 获取ModbusManager单例实例
 * @return ModbusManager实例指针
 */
ModbusManager* ModbusManager::instance()
{
    if (!m_instance) {
        m_instance = new ModbusManager();
    }
    return m_instance;
}

/**
 * @brief 初始化Modbus RTU串行主站通信
 * @param portName 串口名称
 * @param baudRate 波特率
 * @return 初始化是否成功
 */
bool ModbusManager::initModbus(const QString &portName, int baudRate)
{
    // 如果Modbus已经连接，先断开
    if (modbusMaster) {
        if (modbusMaster->state() == QModbusDevice::ConnectedState) {
            modbusMaster->disconnectDevice();
        }
        delete modbusMaster;
        modbusMaster = nullptr;
    }
    
    // 配置串口参数
    COM->setPortName(portName);
    COM->setBaudRate(baudRate);
    COM->setDataBits(QSerialPort::Data8);
    COM->setStopBits(QSerialPort::OneStop);
    COM->setParity(QSerialPort::NoParity);
    
    // 如果全局串口已打开，先关闭它
    if (COM->isOpen()) {
        qDebug() << "关闭全局串口以让Modbus直接访问";
        COM->close();
    }
    
    // 创建Modbus主站实例
    modbusMaster = new QModbusRtuSerialMaster();
    
    // 配置Modbus连接参数
    modbusMaster->setConnectionParameter(QModbusDevice::SerialPortNameParameter, QVariant(portName));
    modbusMaster->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, QVariant(baudRate));
    modbusMaster->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QVariant(QSerialPort::Data8));
    modbusMaster->setConnectionParameter(QModbusDevice::SerialParityParameter, QVariant(QSerialPort::NoParity));
    modbusMaster->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QVariant(QSerialPort::OneStop));
    
    // 设置超时时间（毫秒）- 增加到500ms以提高稳定性
    modbusMaster->setTimeout(400);
    
    // 设置重试次数 - 增加到2次重试
    modbusMaster->setNumberOfRetries(1);
    
    // 建立Modbus连接
    if (!modbusMaster->connectDevice()) {
        qDebug() << "Modbus连接失败:" << modbusMaster->errorString();
        delete modbusMaster;
        modbusMaster = nullptr;
        return false;
    }
    
    // 等待连接建立
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);  // 等待500ms
    loop.exec();
    
    if (modbusMaster->state() == QModbusDevice::ConnectedState) {
        qDebug() << "Modbus连接成功 - 端口:" << portName << ", 波特率:" << baudRate;
        // 重置连接稳定标志
        m_modbusStable = false;
        // 延迟2秒后标记连接稳定，给设备足够的初始化时间
        QTimer::singleShot(500, this, [this]() {
            m_modbusStable = true;
            qDebug() << "Modbus连接已稳定，可以开始正常通信";
        });
        m_serialPortOpen = true;
        return true;
    } else {
        qDebug() << "Modbus连接超时 - 最终状态:" << modbusMaster->state();
        delete modbusMaster;
        modbusMaster = nullptr;
        return false;
    }
}

/**
 * @brief 写入寄存器数据
 * @param address 寄存器地址
 * @param value   要写入的值
 */
void ModbusManager::writeRegister(int address, int value)
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
void ModbusManager::readRegister(int address, std::function<void(int)> callback)
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
 * @brief 读取从站3的寄存器0（电压数据）
 * @param callback 读取完成后的回调函数
 */
void ModbusManager::readSlave3Register0(std::function<void(int)> callback)
{
    // 检查Modbus连接状态
    if (!modbusMaster) {
        callback(-1);
        return;
    }
    
    if (modbusMaster->state() != QModbusDevice::ConnectedState) {
        callback(-1);
        return;
    }
    
    // 检查Modbus连接是否稳定
    if (!m_modbusStable) {
        callback(-1);
        return;
    }
    
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, VOLTAGE_REGISTER_ADDRESS, 1);
    
    if (auto *reply = modbusMaster->sendReadRequest(readUnit, VOLTAGE_SLAVE_ADDRESS)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [reply, callback]() {
                if (reply->error() != QModbusDevice::NoError) {
                    reply->deleteLater();
                    callback(-1);
                    return;
                }
                
                QModbusDataUnit result = reply->result();
                int value = result.value(0);
                
                callback(value);
                
                reply->deleteLater();
            });
        } else {
            reply->deleteLater();
            callback(-1);
        }
    } else {
        callback(-1);
    }
}

/**
 * @brief 关闭Modbus连接
 */
void ModbusManager::closeModbus()
{
    // 关闭Modbus连接
    if (modbusMaster && modbusMaster->state() == QModbusDevice::ConnectedState) {
        modbusMaster->disconnectDevice();
    }
    
    // 关闭串口
    if (COM->isOpen()) {
        COM->close();
    }
    
    // 更新状态标志
    m_serialPortOpen = false;
    m_modbusStable = false;
    
    qDebug() << "Modbus连接已关闭";
}

/**
 * @brief 获取Modbus连接状态
 * @return 是否连接
 */
bool ModbusManager::isConnected() const
{
    return modbusMaster && modbusMaster->state() == QModbusDevice::ConnectedState;
}

/**
 * @brief 获取Modbus连接是否稳定
 * @return 是否稳定
 */
bool ModbusManager::isStable() const
{
    return m_modbusStable;
}
