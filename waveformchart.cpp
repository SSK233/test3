/**
 * @file waveformchart.cpp
 * @brief 波形图类实现文件
 * @details WaveformChart类的实现，用于显示实时电压波形图
 */

#include "waveformchart.h"
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QEvent>
#include <QApplication>

constexpr int WaveformChart::MAX_DATA_POINTS;

CustomChartView::CustomChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
    , m_hoverPoint()
{
}

QPointF CustomChartView::findClosestDataPoint(const QPoint &pos)
{
    QChart *chart = this->chart();
    if (!chart) return QPointF();

    QList<QAbstractSeries*> series = chart->series();
    if (series.isEmpty()) return QPointF();

    QLineSeries *lineSeries = qobject_cast<QLineSeries*>(series.first());
    if (!lineSeries) return QPointF();

    QPointF closestPoint;
    double minDistance = 1e9;

    QPointF valuePos = chart->mapToValue(chart->mapFromScene(mapToScene(pos)));

    for (const QPointF &point : lineSeries->points()) {
        double dx = valuePos.x() - point.x();
        double dy = valuePos.y() - point.y();
        double distance = sqrt(dx * dx + dy * dy);
        if (distance < minDistance) {
            minDistance = distance;
            closestPoint = point;
        }
    }

    return minDistance < 0.5 ? closestPoint : QPointF();
}

QString CustomChartView::formatToolTipText(const QPointF &dataPoint)
{
    QString text = QString("<b>时间:</b> %1 s<br>").arg(dataPoint.x(), 0, 'f', 0);
    text += QString("<b>电压:</b> %2 V").arg(dataPoint.y(), 0, 'f', 3);
    return text;
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF dataPoint = findClosestDataPoint(event->position().toPoint());
    
    bool updateNeeded = false;
    if (dataPoint.isNull()) {
        if (!m_hoverPoint.isNull()) {
            m_hoverPoint = QPointF();
            updateNeeded = true;
        }
        QToolTip::hideText();
    } else {
        if (m_hoverPoint.isNull() || !qFuzzyCompare(m_hoverPoint.x(), dataPoint.x()) || !qFuzzyCompare(m_hoverPoint.y(), dataPoint.y())) {
            m_hoverPoint = dataPoint;
            updateNeeded = true;
        }
        
        QString toolTip = formatToolTipText(dataPoint);
        
        QPoint globalPos = mapToGlobal(event->position().toPoint());
        
        int xOffset = 15;
        int yOffset = 15;
        
        if (globalPos.x() + xOffset + 150 > QApplication::primaryScreen()->availableGeometry().width()) {
            xOffset = -165;
        }
        
        if (globalPos.y() + yOffset + 60 > QApplication::primaryScreen()->availableGeometry().height()) {
            yOffset = -65;
        }
        
        QPoint toolTipPos = globalPos + QPoint(xOffset, yOffset);
        QToolTip::showText(toolTipPos, toolTip, this);
    }
    
    if (updateNeeded) {
        update();
    }

    QChartView::mouseMoveEvent(event);
}

void CustomChartView::paintEvent(QPaintEvent *event)
{
    QChartView::paintEvent(event);
    
    if (!m_hoverPoint.isNull()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        drawDataPointMarker(&painter, m_hoverPoint);
    }
}

void CustomChartView::drawDataPointMarker(QPainter *painter, const QPointF &point)
{
    QChart *chart = this->chart();
    if (!chart) return;
    
    QPointF scenePos = chart->mapToScene(chart->mapToValue(point));
    QPointF widgetPos = mapFromScene(scenePos).toPointF();
    
    painter->save();
    
    QPen pen(Qt::red);
    pen.setWidth(3);
    painter->setPen(pen);
    
    int markerSize = 8;
    painter->drawEllipse(widgetPos, markerSize, markerSize);
    
    QPen haloPen(Qt::white);
    haloPen.setWidth(5);
    painter->setPen(haloPen);
    painter->drawEllipse(widgetPos, markerSize + 2, markerSize + 2);
    
    painter->restore();
}

void CustomChartView::leaveEvent(QEvent *event)
{
    QToolTip::hideText();
    if (!m_hoverPoint.isNull()) {
        m_hoverPoint = QPointF();
        update();
    }
    QChartView::leaveEvent(event);
}

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
    , m_currentTimeWindowStart(0.0)
    , m_updateInterval(1000)
    , m_yAxisMin(0)
    , m_yAxisMax(500)
    , m_title("电压实时波形图")
    , m_useAdaptiveRange(true)
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
    if (!m_useAdaptiveRange) {
        axisY->setRange(m_yAxisMin, m_yAxisMax);
    }
    voltageChart->addAxis(axisY, Qt::AlignLeft);
    voltageSeries->attachAxis(axisY);

    chartView = new CustomChartView(voltageChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QRect containerRect = chartContainer->geometry();
    QRect chartRect = containerRect.adjusted(30, 30, -30, -30);
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
    // 添加新的电压数据点到数据列表
    voltageData.append(voltage);
    // 更新总数据点计数
    m_dataPointCount++;

    // 如果数据点数量超过最大值，移除最旧的数据点并移动时间窗口起始点
    if (voltageData.size() > MAX_DATA_POINTS) {
        voltageData.removeFirst();
        m_currentTimeWindowStart++;
    }

    // 更新图表数据
    if (voltageSeries) {
        voltageSeries->clear();

        double timeWindowEnd = m_currentTimeWindowStart + MAX_DATA_POINTS;
        // 重新添加所有数据点到序列，保持时间窗口的连续性
        for (int i = 0; i < voltageData.size(); ++i) {
            double time = m_currentTimeWindowStart + i;
            voltageSeries->append(time, voltageData[i]);
        }

        // 更新X轴范围，确保时间窗口正确显示
        QValueAxis *axisX = qobject_cast<QValueAxis*>(voltageChart->axisX(voltageSeries));
        if (axisX) {
            double timeWindowEnd = m_currentTimeWindowStart + MAX_DATA_POINTS;
            axisX->setRange(m_currentTimeWindowStart, timeWindowEnd);
        }
    }

    // 如果使用自适应Y轴范围，根据数据自动调整Y轴显示范围
    if (m_useAdaptiveRange && !voltageData.isEmpty()) {
        double minVoltage = voltageData[0];
        double maxVoltage = voltageData[0];

        // 遍历所有数据点，找出最小值和最大值
        for (double v : voltageData) {
            if (v < minVoltage) minVoltage = v;
            if (v > maxVoltage) maxVoltage = v;
        }

        // 计算边距，确保图表显示时留有足够空间
        double margin = (maxVoltage - minVoltage) * 0.1;
        // 保证最小边距为0.5，防止显示范围过小
        if (margin < 0.5) margin = 0.5;

        // 计算新的Y轴显示范围
        double newMin = minVoltage - margin;
        double newMax = maxVoltage + margin;

        // 更新Y轴范围
        QValueAxis *axisY = qobject_cast<QValueAxis*>(voltageChart->axisY(voltageSeries));
        if (axisY) {
            axisY->setRange(newMin, newMax);
        }
    }

    // 发送数据更新信号
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
    m_currentTimeWindowStart = 0.0;

    if (voltageSeries) {
        voltageSeries->clear();
    }

    QValueAxis *axisX = qobject_cast<QValueAxis*>(voltageChart->axisX());
    if (axisX) {
        axisX->setRange(0, MAX_DATA_POINTS);
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
 * @param adaptive 是否使用自适应范围
 */
void WaveformChart::setYAxisRange(double min, double max, bool adaptive)
{
    if (min >= max) {
        qWarning() << "Y轴最小值必须小于最大值";
        return;
    }

    m_yAxisMin = min;
    m_yAxisMax = max;
    m_useAdaptiveRange = adaptive;

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

/**
 * @brief 更新图表大小，使其与容器大小保持同步
 * @param chartContainer 图表容器
 */
void WaveformChart::updateChartSize(QWidget *chartContainer)
{
    if (chartView && chartContainer) {
        QRect containerRect = chartContainer->geometry();
        QRect chartRect = containerRect.adjusted(30, 30, -30, -60);
        chartView->setGeometry(chartRect);
    }
}
