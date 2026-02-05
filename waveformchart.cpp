/**
 * @file waveformchart.cpp
 * @brief 波形图类实现文件
 * @details WaveformChart类的实现，用于显示实时波形图
 */

#include "waveformchart.h"
#include <QDebug>
#include <QPainter>
#include <QMouseEvent>
#include <QEvent>
#include <QApplication>
#include <QWheelEvent>
#include <QElapsedTimer>

constexpr int WaveformChart::MAX_DATA_POINTS;

CustomChartView::CustomChartView(QChart *chart, const QString &unit, QWidget *parent)
    : QChartView(chart, parent)
    , m_hoverPoint()
    , m_unit(unit)
    , m_isPanning(false)
{
    setRubberBand(QChartView::RectangleRubberBand);
    setInteractive(true);
}

QPointF CustomChartView::findClosestDataPoint(const QPoint &pos)
{
    QChart *chart = this->chart();
    if (!chart) return QPointF();

    QList<QAbstractSeries*> seriesList = chart->series();
    if (seriesList.isEmpty()) return QPointF();

    QPointF closestPoint;
    double minDistance = 1e9;

    QPointF valuePos = chart->mapToValue(chart->mapFromScene(mapToScene(pos)));

    for (QAbstractSeries *abstractSeries : seriesList) {
        QLineSeries *lineSeries = qobject_cast<QLineSeries*>(abstractSeries);
        if (!lineSeries || !lineSeries->isVisible()) continue;

        for (const QPointF &point : lineSeries->points()) {
            double dx = valuePos.x() - point.x();
            double dy = valuePos.y() - point.y();
            double distance = sqrt(dx * dx + dy * dy);
            if (distance < minDistance) {
                minDistance = distance;
                closestPoint = point;
            }
        }
    }

    return minDistance < 0.5 ? closestPoint : QPointF();
}

QString CustomChartView::formatToolTipText(const QPointF &dataPoint)
{
    QString text = QString("<b>时间:</b> %1 s<br>").arg(dataPoint.x(), 0, 'f', 0);
    text += QString("<b>数值:</b> %1 %2").arg(dataPoint.y(), 0, 'f', 3).arg(m_unit);
    return text;
}

void CustomChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        chart()->scroll(-delta.x(), delta.y());
        m_lastPanPoint = event->pos();
        return;
    }

    QPointF dataPoint = findClosestDataPoint(event->position().toPoint());
    
    bool updateNeeded = false;
    if (dataPoint.isNull()) {
        if (!m_hoverPoint.isNull()) {
            m_hoverPoint = QPointF();
            updateNeeded = true;
        }
        QToolTip::hideText();
    } else {
        if (m_hoverPoint.isNull() || !qFuzzyCompare(m_hoverPoint.x(), dataPoint.x()) || 
            !qFuzzyCompare(m_hoverPoint.y(), dataPoint.y())) {
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

void CustomChartView::wheelEvent(QWheelEvent *event)
{
    QChart *chart = this->chart();
    if (!chart) return;

    const double zoomFactor = 1.15;
    if (event->angleDelta().y() > 0) {
        chart->zoom(zoomFactor);
    } else {
        chart->zoom(1.0 / zoomFactor);
    }
    event->accept();
}

void CustomChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton || 
        (event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier)) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QChartView::mousePressEvent(event);
}

void CustomChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QChartView::mouseReleaseEvent(event);
}

WaveformChart::WaveformChart(QObject *parent)
    : QObject(parent)
    , m_chart(nullptr)
    , m_series(nullptr)
    , m_chartView(nullptr)
    , m_waveformUpdateTimer(nullptr)
    , m_dataPointCount(0)
    , m_currentTimeWindowStart(0.0)
    , m_updateInterval(1000)
    , m_yAxisMin(0)
    , m_yAxisMax(500)
    , m_useAdaptiveRange(true)
    , m_isInitialized(false)
    , m_title("实时波形图")
    , m_unit("")
    , m_color(Qt::red)
    , m_axisX(nullptr)
    , m_axisY(nullptr)
{
}

WaveformChart::~WaveformChart()
{
    stopWaveformUpdate();
    clearWaveformData();

    if (m_chartView) {
        m_chartView->setParent(nullptr);
        delete m_chartView;
        m_chartView = nullptr;
    }

    if (m_chart) {
        delete m_chart;
        m_chart = nullptr;
    }
}

void WaveformChart::initWaveform(QWidget *chartContainer, QWidget *pageWidget,
                                  const QString &title, const QString &unit,
                                  const QColor &color)
{
    m_title = title;
    m_unit = unit;
    m_color = color;
    
    setupWaveformChart(chartContainer, pageWidget);

    if (m_waveformUpdateTimer) {
        delete m_waveformUpdateTimer;
    }
    m_waveformUpdateTimer = new QTimer(this);
    m_waveformUpdateTimer->setInterval(m_updateInterval);
    
    m_isInitialized = true;
}

void WaveformChart::setupWaveformChart(QWidget *chartContainer, QWidget *pageWidget)
{
    if (m_chartView) {
        m_chartView->setParent(nullptr);
        delete m_chartView;
        m_chartView = nullptr;
    }

    if (m_chart) {
        delete m_chart;
        m_chart = nullptr;
    }

    m_chart = new QChart();
    m_chart->setTitle(m_title);
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->setMargins(QMargins(10, 10, 10, 30));
    m_chart->legend()->setVisible(false);

    m_series = new QLineSeries();
    m_series->setName(m_title);
    m_series->setColor(m_color);
    m_chart->addSeries(m_series);

    m_axisX = new QValueAxis();
    m_axisX->setTitleText("时间 (s)");
    m_axisX->setRange(0, MAX_DATA_POINTS);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText(m_unit);
    m_axisY->setRange(0, 100);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    m_chartView = new CustomChartView(m_chart, m_unit);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setParent(chartContainer);
    m_chartView->setGeometry(0, 0, chartContainer->width(), chartContainer->height());
    m_chartView->show();

    chartContainer->setStyleSheet("background-color: white; border: 1px solid #ccc; border-radius: 5px;");
}

void WaveformChart::updateData(double value)
{
    QElapsedTimer timer;
    timer.start();
    
    m_data.append(value);
    m_dataPointCount++;

    if (m_data.size() > MAX_DATA_POINTS) {
        m_data.removeFirst();
        m_currentTimeWindowStart++;
    }

    if (m_series) {
        m_series->clear();
        for (int i = 0; i < m_data.size(); ++i) {
            double time = m_currentTimeWindowStart + i;
            m_series->append(time, m_data[i]);
        }
    }

    if (m_axisX) {
        double timeWindowEnd = m_currentTimeWindowStart + MAX_DATA_POINTS;
        m_axisX->setRange(m_currentTimeWindowStart, timeWindowEnd);
    }

    updateAdaptiveYAxis();
    
    emit dataUpdated(m_data);
    
    qint64 elapsed = timer.elapsed();
    if (elapsed > 500) {
        qWarning() << "波形数据更新超时:" << elapsed << "ms";
    }
}

void WaveformChart::updateAdaptiveYAxis()
{
    if (!m_axisY || !m_useAdaptiveRange || m_data.isEmpty()) return;

    double minValue = m_data[0];
    double maxValue = m_data[0];

    for (double v : m_data) {
        if (v < minValue) minValue = v;
        if (v > maxValue) maxValue = v;
    }

    double margin = (maxValue - minValue) * 0.1;
    if (margin < 0.5) margin = 0.5;
    
    double newMin = minValue - margin;
    double newMax = maxValue + margin;
    m_axisY->setRange(newMin, newMax);
}

void WaveformChart::startWaveformUpdate()
{
    if (m_waveformUpdateTimer && !m_waveformUpdateTimer->isActive()) {
        m_waveformUpdateTimer->start();
        qDebug() << "波形图更新定时器已启动:" << m_title;
    }
}

void WaveformChart::stopWaveformUpdate()
{
    if (m_waveformUpdateTimer && m_waveformUpdateTimer->isActive()) {
        m_waveformUpdateTimer->stop();
        qDebug() << "波形图更新定时器已停止:" << m_title;
    }
}

void WaveformChart::clearWaveformData()
{
    m_data.clear();
    m_dataPointCount = 0;
    m_currentTimeWindowStart = 0.0;

    if (m_series) m_series->clear();

    if (m_axisX) {
        m_axisX->setRange(0, MAX_DATA_POINTS);
    }
}

void WaveformChart::setUpdateInterval(int interval)
{
    if (interval <= 0) {
        qWarning() << "波形图更新间隔必须为正数";
        return;
    }

    m_updateInterval = interval;

    if (m_waveformUpdateTimer) {
        bool wasActive = m_waveformUpdateTimer->isActive();
        m_waveformUpdateTimer->setInterval(interval);
        if (wasActive) {
            m_waveformUpdateTimer->start(interval);
        }
    }
}

void WaveformChart::setYAxisRange(double min, double max, bool adaptive)
{
    if (min >= max) {
        qWarning() << "Y轴最小值必须小于最大值";
        return;
    }

    m_yAxisMin = min;
    m_yAxisMax = max;
    m_useAdaptiveRange = adaptive;

    if (!adaptive && m_axisY) {
        m_axisY->setRange(min, max);
    }
}

int WaveformChart::dataPointCount() const
{
    return m_dataPointCount;
}

void WaveformChart::setTitle(const QString &title)
{
    m_title = title;

    if (m_chart) {
        m_chart->setTitle(title);
    }
}

void WaveformChart::updateChartSize(QWidget *chartContainer)
{
    if (m_chartView && chartContainer) {
        m_chartView->setGeometry(0, 0, chartContainer->width(), chartContainer->height());
    }
}

void WaveformChart::setVisible(bool visible)
{
    if (m_chartView) {
        m_chartView->setVisible(visible);
    }
}

bool WaveformChart::isVisible() const
{
    return m_chartView ? m_chartView->isVisible() : false;
}

void WaveformChart::zoomIn()
{
    if (m_chart) {
        m_chart->zoomIn();
    }
}

void WaveformChart::zoomOut()
{
    if (m_chart) {
        m_chart->zoomOut();
    }
}

void WaveformChart::zoomReset()
{
    if (m_chart) {
        m_chart->zoomReset();
    }
}

void WaveformChart::pan(double dx, double dy)
{
    if (m_chart) {
        m_chart->scroll(dx, dy);
    }
}
