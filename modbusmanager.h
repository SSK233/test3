#ifndef MODBUSMANAGER_H
#define MODBUSMANAGER_H

#include <QObject>
#include <QModbusRtuSerialMaster>
#include <QSerialPort>
#include <functional>

class ModbusManager : public QObject
{
    Q_OBJECT

public:
    explicit ModbusManager(QObject *parent = nullptr);
    ~ModbusManager();

    // 初始化Modbus RTU通信
    bool initModbus(const QString &portName, int baudRate = 9600);
    
    // 写入寄存器数据
    void writeRegister(int address, int value);
    
    // 读取寄存器数据
    void readRegister(int address, std::function<void(int)> callback);
    
    // 读取从站3的寄存器7（电压数据）
    void readSlave3Register7(std::function<void(int)> callback);
    
    // 关闭Modbus连接
    void closeModbus();
    
    // 获取Modbus连接状态
    bool isConnected() const;
    
    // 获取Modbus连接是否稳定
    bool isStable() const;
    
    // 静态实例获取方法
    static ModbusManager* instance();

private:
    QModbusRtuSerialMaster *modbusMaster;
    QSerialPort *COM;
    bool m_serialPortOpen;
    bool m_modbusStable;
    
    // 静态单例实例
    static ModbusManager* m_instance;
};

#endif // MODBUSMANAGER_H