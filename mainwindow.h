/**
 * @file mainwindow.h
 * @brief 主窗口类定义文件
 * @details 包含MainWindow类的声明，负责整个应用程序的界面管理和主要功能实现
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QVector>
#include <QLocale>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QRadioButton>
#include <QTimer>
#include <functional>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>

#include "rowbuttongroup.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow;

/**
 * @brief 寄存器地址常量定义
 * @details 定义各行列对应的Modbus寄存器地址
 */
constexpr int REGISTER_ADDRESS_ROW0 = 50;  // 第0行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW1 = 1;   // 第1行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW2 = 2;   // 第2行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW3 = 3;   // 第3行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW4 = 4;   // 第4行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW5 = 5;   // 第5行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW6 = 6;   // 第6行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW7 = 7;   // 第7行对应的寄存器地址
constexpr int REGISTER_ADDRESS_ROW8 = 8;   // 第8行对应的寄存器地址

/**
 * @class MainWindow
 * @brief 主窗口类
 * @details 继承自QMainWindow，负责应用程序的主界面显示和交互逻辑
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief "刷新串口"按钮点击事件处理函数
     */
    void on_key_Refresh_COM_clicked();
    
    /**
     * @brief "启动串口"按钮点击事件处理函数
     */
    void on_key_OpenOrClose_COM_clicked();
    
    /**
     * @brief textBrowser文本变化处理函数
     */
    void on_textBrowser_textChanged();

private:
    Ui::MainWindow *ui;                  // UI界面指针
    RowButtonGroup row0, row1, row2, row3, row4, row5, row6, row7, row8;  // 行按钮组对象
    static bool m_serialPortOpen;        // 串口状态标志
    QTimer *refreshTimer;                // 刷新定时器
    QTimer *slave3Timer;                 // 从机3定时器
    
    // 波形图相关成员变量
    QChart *voltageChart;                // 电压波形图表
    QLineSeries *voltageSeries;          // 电压数据系列
    QChartView *chartView;               // 图表视图
    QTimer *waveformUpdateTimer;         // 波形更新定时器
    QVector<double> voltageData;         // 电压数据存储
    int dataPointCount;                  // 数据点计数
    static constexpr int MAX_DATA_POINTS = 500;  // 最大数据点数量

public:
    /**
     * @brief 刷新所有行的数据
     */
    void refreshAllRows();
    
    /**
     * @brief 刷新指定行的数据
     * @param rowIndex 行索引
     */
    void refreshRow(int rowIndex);
    
    /**
     * @brief 读取从机3的寄存器7
     */
    void readSlave3Register7();
    
    /**
     * @brief 初始化电压波形图
     */
    void initVoltageWaveform();
    
    /**
     * @brief 切换到波形图页面
     */
    void switchToWaveformPage();
    
    /**
     * @brief 切换到主页面
     */
    void switchToMainPage();
    
    /**
     * @brief 更新波形图数据
     * @param voltage 电压值
     */
    void updateWaveformData(double voltage);
    
    /**
     * @brief 设置波形图图表
     */
    void setupWaveformChart();
    
    /**
     * @brief 清空指定行的数据
     * @param rowIndex 行索引
     */
    void clearRow(int rowIndex);
    
    /**
     * @brief 暂停刷新定时器
     */
    void pauseRefreshTimer();
    
    /**
     * @brief 恢复刷新定时器
     */
    void resumeRefreshTimer();
};

#endif // MAINWINDOW_H
