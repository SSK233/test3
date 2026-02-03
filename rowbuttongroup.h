#ifndef ROWBUTTONGROUP_H
#define ROWBUTTONGROUP_H

#include <QObject>
#include <QPushButton>
#include <QLineEdit>
#include <QVector>
#include <QTimer>

class MainWindow;

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

#endif // ROWBUTTONGROUP_H
