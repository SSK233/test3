/**
 * @file waveformchart.cpp
 * @brief 波形图类实现文件
 * @details WaveformChart类的实现，用于显示实时电压波形图
 */

#include "waveformchart.h"
#include <QDebug>
#include <QPainter>

constexpr int WaveformChart::MAX_DATA_POINTS;

/**
 * @brief 构造函数
 * @param parent 父对象指针
 */
WaveformChart::WaveformChart(QObject *parent)
    : QObject(parent)
    , voltageChart(nullptr)
    , voltageSeries(nullptr)
    , chartView(nullptr)
    , waveformUpdateTimer(nullptr)
    , m_dataPointCount(0)
    , m_updateInterval(1000)
    , m_yAxisMin(225.0)
    , m_yAxisMax(235.0)
    , m_title("电压实时波形图")
{
}

/**
 * @brief 析构函数
 */
WaveformChart::~WaveformChart()
{
    stopWaveformUpdate();
    clearWaveformData();

    if (chartView) {
        chartView->setParent(nullptr);
        delete chartView;
        chartView = nullptr;
    }

    if (voltageChart) {
        delete voltageChart;
        voltageChart = nullptr;
    }
}

/**
 * @brief 初始化电压波形图
 * @param chartContainer 用于放置图表的容器
 * @param pageWidget 包含图表容器的页面
 */
void WaveformChart::initVoltageWaveform(QWidget *chartContainer, QWidget *pageWidget)
{
    setupWaveformChart(chartContainer, pageWidget);

    if (waveformUpdateTimer) {
        delete waveformUpdateTimer;
    }
    waveformUpdateTimer = new QTimer(this);
    waveformUpdateTimer->setInterval(m_updateInterval);
}

/**
 * @brief 设置波形图图表
 * @param chartContainer 用于放置图表的容器
 * @param pageWidget 包含图表容器的页面
 */
void WaveformChart::setupWaveformChart(QWidget *chartContainer, QWidget *pageWidget)
{
    if (chartView) {
        chartView->setParent(nullptr);
        delete chartView;
    }

    if (voltageChart) {
        delete voltageChart;
    }

    voltageChart = new QChart();
    voltageChart->setTitle(m_title);
    voltageChart->setAnimationOptions(QChart::NoAnimation);
    voltageChart->setMargins(QMargins(10, 10, 10, 30));
    voltageChart->legend()->setVisible(true);

    voltageSeries = new QLineSeries();
    voltageSeries->setName("电压 (V)");
    voltageChart->addSeries(voltageSeries);

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("时间 (s)");
    axisX->setRange(0, MAX_DATA_POINTS);
    voltageChart->addAxis(axisX, Qt::AlignBottom);
    voltageSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("电压 (V)");
    axisY->setRange(m_yAxisMin, m_yAxisMax);
    voltageChart->addAxis(axisY, Qt::AlignLeft);
    voltageSeries->attachAxis(axisY);

    chartView = new QChartView(voltageChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QRect containerRect = chartContainer->geometry();
    QRect chartRect = containerRect.adjusted(30, 30, -30, -80);
    chartView->setGeometry(chartRect);
    chartView->setParent(pageWidget);

    chartContainer->setStyleSheet("background-color: white;");
}

/**
 * @brief 更新波形图数据
 * @param voltage 电压值
 */
void WaveformChart::updateWaveformData(double voltage)
{
    voltageData.append(voltage);
    m_dataPointCount++;

    if (voltageData.size() > MAX_DATA_POINTS) {
        voltageData.removeFirst();
    }

    if (voltageSeries) {
        voltageSeries->clear();

        for (int i = 0; i < voltageData.size(); ++i) {
            voltageSeries->append(i, voltageData[i]);
        }
    }

    emit dataUpdated(voltageData);
}

/**
 * @brief 启动波形图更新定时器
 */
void WaveformChart::startWaveformUpdate()
{
    if (waveformUpdateTimer && !waveformUpdateTimer->isActive()) {
        waveformUpdateTimer->start();
        qDebug() << "波形图更新定时器已启动";
    }
}

/**
 * @brief 停止波形图更新定时器
 */
void WaveformChart::stopWaveformUpdate()
{
    if (waveformUpdateTimer && waveformUpdateTimer->isActive()) {
        waveformUpdateTimer->stop();
        qDebug() << "波形图更新定时器已停止";
    }
}

/**
 * @brief 清除所有波形图数据
 */
void WaveformChart::clearWaveformData()
{
    voltageData.clear();
    m_dataPointCount = 0;

    if (voltageSeries) {
        voltageSeries->clear();
    }
}

/**
 * @brief 设置波形图更新间隔
 * @param interval 间隔时间（毫秒）
 */
void WaveformChart::setUpdateInterval(int interval)
{
    if (interval <= 0) {
        qWarning() << "波形图更新间隔必须为正数";
        return;
    }

    m_updateInterval = interval;

    if (waveformUpdateTimer) {
        bool wasActive = waveformUpdateTimer->isActive();
        waveformUpdateTimer->setInterval(interval);
        if (wasActive) {
            waveformUpdateTimer->start(interval);
        }
    }
}

/**
 * @brief 设置Y轴范围
 * @param min 最小值
 * @param max 最大值
 */
void WaveformChart::setYAxisRange(double min, double max)
{
    if (min >= max) {
        qWarning() << "Y轴最小值必须小于最大值";
        return;
    }

    m_yAxisMin = min;
    m_yAxisMax = max;

    if (voltageChart) {
        QAbstractAxis *axisY = voltageChart->axisY();
        QValueAxis *valueAxisY = qobject_cast<QValueAxis*>(axisY);
        if (valueAxisY) {
            valueAxisY->setRange(min, max);
        }
    }
}

/**
 * @brief 获取当前数据点数量
 * @return 数据点数量
 */
int WaveformChart::dataPointCount() const
{
    return m_dataPointCount;
}

/**
 * @brief 设置波形图标题
 * @param title 标题文本
 */
void WaveformChart::setTitle(const QString &title)
{
    m_title = title;

    if (voltageChart) {
        voltageChart->setTitle(title);
    }
}
