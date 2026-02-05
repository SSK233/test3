/**
 * @file waveformchart.h
 * @brief 波形图类头文件
 * @details 包含WaveformChart类的定义，用于显示实时波形图（电压/电流/功率）
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
#include <QColor>

class CustomChartView : public QChartView
{
    Q_OBJECT

public:
    explicit CustomChartView(QChart *chart, const QString &unit = QString(), QWidget *parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPointF findClosestDataPoint(const QPoint &pos);
    QString formatToolTipText(const QPointF &dataPoint);
    void drawDataPointMarker(QPainter *painter, const QPointF &point);

private:
    QPointF m_hoverPoint;
    QString m_unit;
    bool m_isPanning;
    QPoint m_lastPanPoint;
};

class WaveformChart : public QObject
{
    Q_OBJECT

public:
    explicit WaveformChart(QObject *parent = nullptr);
    virtual ~WaveformChart();

    void initWaveform(QWidget *chartContainer, QWidget *pageWidget,
                      const QString &title, const QString &unit,
                      const QColor &color);

    void updateData(double value);
    void startWaveformUpdate();
    void stopWaveformUpdate();
    void clearWaveformData();

    void setUpdateInterval(int interval);
    void setYAxisRange(double min, double max, bool adaptive = true);
    int dataPointCount() const;
    void setTitle(const QString &title);
    void updateChartSize(QWidget *chartContainer);

    void setVisible(bool visible);
    bool isVisible() const;

    void zoomIn();
    void zoomOut();
    void zoomReset();
    void pan(double dx, double dy);

signals:
    void dataUpdated(const QVector<double> &data);

private:
    void setupWaveformChart(QWidget *chartContainer, QWidget *pageWidget);
    void updateAdaptiveYAxis();

private:
    QChart *m_chart;
    QLineSeries *m_series;
    CustomChartView *m_chartView;
    QTimer *m_waveformUpdateTimer;

    QVector<double> m_data;
    int m_dataPointCount;
    static constexpr int MAX_DATA_POINTS = 50;
    double m_currentTimeWindowStart;
    int m_updateInterval;

    double m_yAxisMin;
    double m_yAxisMax;
    bool m_useAdaptiveRange;
    bool m_isInitialized;

    QString m_title;
    QString m_unit;
    QColor m_color;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
};

#endif // WAVEFORMCHART_H
