#include "CurveEditor.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

CurveEditor::CurveEditor(QColor lineColor, QWidget *parent)
    : QWidget(parent), m_color(lineColor)
{
    setMinimumSize(180, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pts = {{0.0f, 0.0f}, {1.0f, 1.0f}};
}

QPointF CurveEditor::toWidget(const CurvePoint &p) const {
    float margin = 10.0f;
    float w = width() - 2*margin, h = height() - 2*margin;
    return { margin + p.x * w, margin + (1.0f - p.y) * h };
}

CurvePoint CurveEditor::fromWidget(const QPointF &q) const {
    float margin = 10.0f;
    float w = width() - 2*margin, h = height() - 2*margin;
    float x = (q.x() - margin) / w;
    float y = 1.0f - (q.y() - margin) / h;
    x = std::max(0.0f, std::min(1.0f, x));
    y = std::max(0.0f, std::min(1.0f, y));
    return {x, y};
}

int CurveEditor::hitTest(const QPointF &pos) const {
    for (int i = 0; i < (int)m_pts.size(); i++) {
        QPointF wp = toWidget(m_pts[i]);
        if (std::hypot(wp.x() - pos.x(), wp.y() - pos.y()) < 8.0)
            return i;
    }
    return -1;
}

void CurveEditor::sortPoints() {
    std::sort(m_pts.begin(), m_pts.end(),
              [](const CurvePoint &a, const CurvePoint &b){ return a.x < b.x; });
}

/* Monotone cubic Catmull-Rom evaluation (mirrors color_math.c logic) */
float CurveEditor::evalAt(float t) const {
    int n = (int)m_pts.size();
    if (n <= 1) return t;
    if (t <= m_pts[0].x) return m_pts[0].y;
    if (t >= m_pts[n-1].x) return m_pts[n-1].y;
    int i = 0;
    while (i < n - 2 && t > m_pts[i+1].x) i++;
    float dx = m_pts[i+1].x - m_pts[i].x;
    if (dx < 1e-6f) return m_pts[i].y;
    float u = (t - m_pts[i].x) / dx;
    float m0 = (i == 0)
        ? (m_pts[i+1].y - m_pts[i].y) / dx
        : 0.5f * ((m_pts[i+1].y - m_pts[i].y) / dx + (m_pts[i].y - m_pts[i-1].y) / (m_pts[i].x - m_pts[i-1].x));
    float m1 = (i == n-2)
        ? (m_pts[i+1].y - m_pts[i].y) / dx
        : 0.5f * ((m_pts[i+2].y - m_pts[i+1].y) / (m_pts[i+2].x - m_pts[i+1].x) + (m_pts[i+1].y - m_pts[i].y) / dx);
    float u2 = u*u, u3 = u2*u;
    return (2*u3-3*u2+1)*m_pts[i].y + (u3-2*u2+u)*dx*m0
         + (-2*u3+3*u2)*m_pts[i+1].y + (u3-u2)*dx*m1;
}

void CurveEditor::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    /* Background */
    p.fillRect(rect(), QColor(30, 30, 30));

    /* Grid */
    p.setPen(QColor(60, 60, 60));
    for (int i = 1; i < 4; i++) {
        float x = width() * i / 4.0f, y = height() * i / 4.0f;
        p.drawLine(QPointF(x, 0), QPointF(x, height()));
        p.drawLine(QPointF(0, y), QPointF(width(), y));
    }

    /* Diagonal reference */
    p.setPen(QPen(QColor(80, 80, 80), 1, Qt::DashLine));
    p.drawLine(toWidget({0,0}), toWidget({1,1}));

    /* Curve */
    QPainterPath path;
    const int steps = 200;
    for (int i = 0; i <= steps; i++) {
        float t = i / (float)steps;
        float v = evalAt(t);
        QPointF pt = toWidget({t, v});
        i == 0 ? path.moveTo(pt) : path.lineTo(pt);
    }
    p.setPen(QPen(m_color, 2));
    p.drawPath(path);

    /* Control points */
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(50,50,50), 1));
    for (auto &pt : m_pts) {
        QPointF wp = toWidget(pt);
        p.drawEllipse(wp, 5, 5);
    }
}

void CurveEditor::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_drag = hitTest(e->position());
        if (m_drag < 0 && m_pts.size() < 16) {
            m_pts.push_back(fromWidget(e->position()));
            sortPoints();
            m_drag = hitTest(e->position());
            emit curveChanged();
        }
        update();
    }
}

void CurveEditor::mouseMoveEvent(QMouseEvent *e) {
    if (m_drag >= 0 && (e->buttons() & Qt::LeftButton)) {
        CurvePoint np = fromWidget(e->position());
        /* Prevent crossing neighbours */
        if (m_drag > 0)              np.x = std::max(np.x, m_pts[m_drag-1].x + 0.01f);
        if (m_drag < (int)m_pts.size()-1) np.x = std::min(np.x, m_pts[m_drag+1].x - 0.01f);
        /* Lock endpoints to their x */
        if (m_drag == 0)             np.x = 0.0f;
        if (m_drag == (int)m_pts.size()-1) np.x = 1.0f;
        m_pts[m_drag] = np;
        update();
        emit curveChanged();
    }
}

void CurveEditor::mouseReleaseEvent(QMouseEvent *) { m_drag = -1; }

void CurveEditor::mouseDoubleClickEvent(QMouseEvent *e) {
    /* Double-click an existing point (not endpoints) to remove it */
    int idx = hitTest(e->position());
    if (idx > 0 && idx < (int)m_pts.size()-1) {
        m_pts.erase(m_pts.begin() + idx);
        m_drag = -1;
        update();
        emit curveChanged();
    }
}

void CurveEditor::setPoints(const std::vector<CurvePoint> &pts) {
    m_pts = pts;
    sortPoints();
    update();
}

void CurveEditor::exportToParams(float *cx, float *cy, int *n, int max_pts) const {
    int count = std::min((int)m_pts.size(), max_pts);
    for (int i = 0; i < count; i++) { cx[i] = m_pts[i].x; cy[i] = m_pts[i].y; }
    *n = count;
}

void CurveEditor::importFromParams(const float *cx, const float *cy, int n) {
    m_pts.clear();
    for (int i = 0; i < n; i++) m_pts.push_back({cx[i], cy[i]});
    update();
}
