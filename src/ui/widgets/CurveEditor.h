#pragma once
#include <QWidget>
#include <vector>

struct CurvePoint { float x, y; };

class CurveEditor : public QWidget {
    Q_OBJECT
public:
    explicit CurveEditor(QColor lineColor = Qt::white, QWidget *parent = nullptr);

    void  setPoints(const std::vector<CurvePoint> &pts);
    std::vector<CurvePoint> points() const { return m_pts; }

    /* Fill ColorParams curve arrays for curve index c (0=master,1=R,2=G,3=B) */
    void  exportToParams(float *cx, float *cy, int *n, int max_pts = 16) const;
    void  importFromParams(const float *cx, const float *cy, int n);

signals:
    void curveChanged();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    std::vector<CurvePoint> m_pts;
    int   m_drag = -1;
    QColor m_color;

    QPointF toWidget(const CurvePoint &p) const;
    CurvePoint fromWidget(const QPointF &q) const;
    float evalAt(float t) const;
    void  sortPoints();
    int   hitTest(const QPointF &pos) const;
};
