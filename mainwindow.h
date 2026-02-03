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

#include "RowButtonGroup.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow;

constexpr int REGISTER_ADDRESS_ROW0 = 50;
constexpr int REGISTER_ADDRESS_ROW1 = 1;
constexpr int REGISTER_ADDRESS_ROW2 = 2;
constexpr int REGISTER_ADDRESS_ROW3 = 3;
constexpr int REGISTER_ADDRESS_ROW4 = 4;
constexpr int REGISTER_ADDRESS_ROW5 = 5;
constexpr int REGISTER_ADDRESS_ROW6 = 6;
constexpr int REGISTER_ADDRESS_ROW7 = 7;
constexpr int REGISTER_ADDRESS_ROW8 = 8;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_key_Refresh_COM_clicked();//声明按键“刷新串口”点击函数
    void on_key_OpenOrClose_COM_clicked();//声明按键“启动串口”点击函数
    void on_textBrowser_textChanged();//声明textBrowser文本变化处理函数
private:
    Ui::MainWindow *ui;

    RowButtonGroup row0, row1, row2, row3, row4, row5, row6, row7, row8;
    static bool m_serialPortOpen;  // 串口状态标志
    QTimer *refreshTimer;
    QTimer *slave3Timer;
    
    // 波形图相关
    QChart *voltageChart;
    QLineSeries *voltageSeries;
    QChartView *chartView;
    QTimer *waveformUpdateTimer;
    QVector<double> voltageData;
    int dataPointCount;
    static constexpr int MAX_DATA_POINTS = 500;

public:
    void refreshAllRows();
    void refreshRow(int rowIndex);
    void readSlave3Register7();
    
    // 波形图相关方法
    void initVoltageWaveform();
    void switchToWaveformPage();
    void switchToMainPage();
    void updateWaveformData(double voltage);
    void setupWaveformChart();
    
    void clearRow(int rowIndex);
    
    void pauseRefreshTimer();
    void resumeRefreshTimer();
};
#endif // MAINWINDOW_H
