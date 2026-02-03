/**
 * @file waveformchart.h
 * @brief 波形图类头文件
 * @details 包含WaveformChart类的定义，用于显示实时电压波形图
 */

#ifndef WAVEFORMCHART_H
#define WAVEFORMCHART_H

#include <QObject>
#include <QWidget>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>
#include <QVector>
#include <QTimer>
#include <QToolTip>
#include <QPoint>

class CustomChartView : public QChartView
{
    Q_OBJECT

public:
    explicit CustomChartView(QChart *chart, QWidget *parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPointF findClosestDataPoint(const QPoint &pos);
    QString formatToolTipText(const QPointF &dataPoint);
    void drawDataPointMarker(QPainter *painter, const QPointF &point);

private:
    QPointF m_hoverPoint;
};

class WaveformChart : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit WaveformChart(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~WaveformChart();

    /**
     * @brief 初始化电压波形图
     * @param chartContainer 用于放置图表的容器
     * @param pageWidget 包含图表容器的页面
     */
    void initVoltageWaveform(QWidget *chartContainer, QWidget *pageWidget);

    /**
     * @brief 更新波形图数据
     * @param voltage 电压值
     */
    void updateWaveformData(double voltage);

    /**
     * @brief 启动波形图更新定时器
     */
    void startWaveformUpdate();

    /**
     * @brief 停止波形图更新定时器
     */
    void stopWaveformUpdate();

    /**
     * @brief 清除所有波形图数据
     */
    void clearWaveformData();

    /**
     * @brief 设置波形图更新间隔
     * @param interval 间隔时间（毫秒）
     */
    void setUpdateInterval(int interval);

    /**
     * @brief 设置Y轴范围
     * @param min 最小值
     * @param max 最大值
     * @param adaptive 是否使用自适应范围（默认为true）
     */
    void setYAxisRange(double min, double max, bool adaptive = true);

    /**
     * @brief 获取当前数据点数量
     * @return 数据点数量
     */
    int dataPointCount() const;

    /**
     * @brief 设置波形图标题
     * @param title 标题文本
     */
    void setTitle(const QString &title);

signals:
    /**
     * @brief 波形图数据更新信号
     * @param voltageData 电压数据数组
     */
    void dataUpdated(const QVector<double> &voltageData);

private:
    /**
     * @brief 设置波形图图表
     * @param chartContainer 用于放置图表的容器
     * @param pageWidget 包含图表容器的页面
     */
    void setupWaveformChart(QWidget *chartContainer, QWidget *pageWidget);

private:
    QChart *voltageChart;
    QLineSeries *voltageSeries;
    QChartView *chartView;
    QTimer *waveformUpdateTimer;
    QVector<double> voltageData;
    int m_dataPointCount;
    static constexpr int MAX_DATA_POINTS = 50;
    double m_currentTimeWindowStart;
    int m_updateInterval;
    double m_yAxisMin;
    double m_yAxisMax;
    QString m_title;
    bool m_useAdaptiveRange;
};

#endif // WAVEFORMCHART_H
