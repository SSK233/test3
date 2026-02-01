#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QVector>
#include <QLocale>
#include <QModbusRtuSerialMaster>
#include <QModbusDataUnit>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QRadioButton>
#include <QTimer>
#include <functional>

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

class RowButtonGroup : public QObject
{
    Q_OBJECT

public:
    RowButtonGroup(QObject *parent = nullptr);

    void initialize(QPushButton *btn0_1, QPushButton *btn0_2, QPushButton *btn0_2_2,
                   QPushButton *btn0_5, QPushButton *btn1, QPushButton *btn2,
                   QPushButton *btn2_2, QPushButton *btn5, QLineEdit *lineEdit, MainWindow *mainWindow, int rowIndex, int address);
public:
    QVector<bool> states;  
    QLineEdit *lineEdit;   
    QSet<int> recentlyChangedRegisters;  // 跟踪最近修改的寄存器地址
    int registerAddress;    // 寄存器地址
    
    void applyButtonStatesToUI(); 

private slots:
    void onButtonClicked();
    void onLineEditTextChanged(const QString &text);

private:
    QVector<QPushButton*> buttons;
    QVector<double> values;
    MainWindow *mainWindow;

    void solveButtonStates(double targetSum);
    void solveCombinations(int target, const QVector<int> &values, int index, QVector<bool> &used, QVector<bool> &bestUsed, int &bestCount);

public:
    bool m_isUpdating;
    int rowIndex;
    void updateSumDisplay();
    
    QTimer *editTimer;
    bool isEditing;
};

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
    static QModbusRtuSerialMaster *modbusMaster;
    QTimer *refreshTimer;
    QTimer *slave3Timer;
    
    void initModbus();
    void refreshAllRows();
    void refreshRow(int rowIndex);
    void readSlave3Register7();
    
public:
    static void writeRegister(int address, int value);
    void clearRow(int rowIndex);
    void readRegister(int address, std::function<void(int)> callback);
};
#endif // MAINWINDOW_H
