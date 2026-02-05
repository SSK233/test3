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
#include <QResizeEvent>
#include <functional>

#include "rowbuttongroup.h"
#include "waveformchart.h"

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
constexpr int REGISTER_ADDRESS_ROW0 = 1;  // 电流按钮1~64行对应的寄存器地址，写入电流的寄存器地址
constexpr int REGISTER_ADDRESS_LOAD_UNLOAD = 0;  // 载入和卸载按钮对应的寄存器地址

/**
 * @class MainWindow
 * @brief 主窗口类
 * @details 继承自QMainWindow，负责应用程序的主界面显示和交互逻辑
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    /**
     * @brief 处理窗口大小变化事件
     * @param event 大小变化事件
     */
    void resizeEvent(QResizeEvent *event) override;

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
     * @brief voltageTextBrowser文本变化处理函数
     */
    void on_voltageTextBrowser_textChanged();

private:
    Ui::MainWindow *ui;                  // UI界面指针
    RowButtonGroup row0;                 // 行按钮组对象
    static bool m_serialPortOpen;        // 串口状态标志
    QTimer *refreshTimer;                // 刷新定时器
    QTimer *slave3Timer;                 // 从机3定时器
    WaveformChart *m_waveformChart;

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
     * @brief 读取从机3的寄存器0
     */
    void readSlave3Register0();
    
    /**
     * @brief 切换到波形图页面
     */
    void switchToWaveformPage();
    
    /**
     * @brief 切换到主页面
     */
    void switchToMainPage();
    
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
