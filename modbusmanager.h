/**
 * @file modbusmanager.h
 * @brief Modbus通信管理类定义文件
 * @details 包含ModbusManager类的声明，负责处理Modbus RTU串行通信，实现单例模式
 */

#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <QObject>
#include <QModbusRtuSerialMaster>
#include <QSerialPort>
#include <functional>

/**
 * @brief 电压读取相关常量定义
 * @details 定义读取电压数据的Modbus配置
 */
constexpr int VOLTAGE_SLAVE_ADDRESS = 3;      // 电压数据的从站地址
constexpr int VOLTAGE_REGISTER_ADDRESS = 7;   // 电压数据的寄存器地址

/**
 * @class ModbusManager
 * @brief Modbus通信管理类
 * @details 负责Modbus RTU串行通信的初始化、读写寄存器、连接状态管理等功能，使用单例模式
 */
class ModbusManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ModbusManager(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ModbusManager();

    /**
     * @brief 初始化Modbus RTU通信
     * @param portName 串口名称
     * @param baudRate 波特率，默认为9600
     * @return 初始化是否成功
     */
    bool initModbus(const QString &portName, int baudRate = 9600);
    
    /**
     * @brief 写入寄存器数据
     * @param address 寄存器地址
     * @param value 要写入的值
     */
    void writeRegister(int address, int value);
    
    /**
     * @brief 读取寄存器数据
     * @param address 寄存器地址
     * @param callback 回调函数，用于处理读取结果
     */
    void readRegister(int address, std::function<void(int)> callback);
    
    /**
     * @brief 读取从站3的寄存器7（电压数据）
     * @param callback 回调函数，用于处理读取结果
     */
    void readSlave3Register7(std::function<void(int)> callback);
    
    /**
     * @brief 关闭Modbus连接
     */
    void closeModbus();
    
    /**
     * @brief 获取Modbus连接状态
     * @return 连接是否已建立
     */
    bool isConnected() const;
    
    /**
     * @brief 获取Modbus连接是否稳定
     * @return 连接是否稳定
     */
    bool isStable() const;
    
    /**
     * @brief 静态实例获取方法
     * @return ModbusManager单例实例
     */
    static ModbusManager* instance();

private:
    QModbusRtuSerialMaster *modbusMaster;  // Modbus RTU主站对象
    QSerialPort *COM;                      // 串口对象
    bool m_serialPortOpen;                 // 串口状态标志
    bool m_modbusStable;                   // Modbus连接稳定标志
    
    static ModbusManager* m_instance;      // 静态单例实例
};

#endif // MODBUSMANAGER_H