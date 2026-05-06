#include "ColorWheel.h"
#include "LabeledSlider.h"   /* for ResettableSlider */
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLabel>
#include <QGridLayout>
#include <QConicalGradient>
#include <QRadialGradient>
#include <cmath>

ColorWheel::ColorWheel(const QString &title, QWidget *parent) : QWidget(parent) {
    setMinimumSize(160, 200);

    m_title = new QLabel(title, this);
    m_title->setAlignment(Qt::AlignCenter);
    QFont f = m_title->font();
    f.setBold(true);
    m_title->setFont(f);

    m_lumSlider = new ResettableSlider(Qt::Vertical, this);
    m_lumSlider->setRange(-1000, 1000);
    m_lumSlider->setValue(0);
    m_lumSlider->setToolTip("Luminance (double-click to reset)");

    m_lumValue = new QLabel("0.00", this);
    m_lumValue->setAlignment(Qt::AlignCenter);
    QFont vf = m_lumValue->font();
    vf.setPointSizeF(vf.pointSizeF() * 0.9);
    m_lumValue->setFont(vf);
    m_lumValue->setMinimumWidth(36);

    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(4);
    grid->addWidget(m_title, 0, 0, 1, 2);
    /* Wheel area is implicit — paintEvent draws into row 1 col 0. */
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    grid->addWidget(spacer,      1, 0);
    grid->addWidget(m_lumSlider, 1, 1);
    grid->addWidget(m_lumValue,  2, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 0);
    grid->setRowStretch(1, 1);

    connect(m_lumSlider, &QSlider::valueChanged, this, [this](int v) {
        m_lum = v / 1000.0f;
        m_lumValue->setText(QString::number(m_lum, 'f', 2));
        emit valuesChanged(m_x, m_y, m_lum);
    });
    connect(m_lumSlider, &ResettableSlider::doubleClicked, this, [this]() {
        m_lumSlider->setValue(0);   /* triggers valueChanged → lum=0, label, signal */
    });
}

void ColorWheel::setValues(float x, float y, float lum, bool silent) {
    /* Clamp dot to unit disk. */
    float r2 = x*x + y*y;
    if (r2 > 1.0f) { float r = std::sqrt(r2); x /= r; y /= r; }
    m_x = x; m_y = y; m_lum = lum;
    m_lumSlider->blockSignals(true);
    m_lumSlider->setValue((int)(lum * 1000.0f + 0.5f));
    m_lumSlider->blockSignals(false);
    m_lumValue->setText(QString::number(m_lum, 'f', 2));
    update();
    if (!silent) emit valuesChanged(m_x, m_y, m_lum);
}

void ColorWheel::reset() { setValues(0, 0, 0, false); }

QRectF ColorWheel::wheelRect() const {
    /* The wheel occupies the spacer cell at row 1 col 0. Use the slider's
     * geometry as the reference so the wheel always lines up vertically
     * with the slider (which doesn't include the value label below it). */
    int top    = m_lumSlider->y();
    int bottom = m_lumSlider->y() + m_lumSlider->height();
    int right  = m_lumSlider->x() - 8;
    QRect avail(4, top, right - 4, bottom - top);
    int side = std::min(avail.width(), avail.height());
    int cx = avail.x() + avail.width() / 2;
    int cy = avail.y() + avail.height() / 2;
    return QRectF(cx - side / 2.0, cy - side / 2.0, side, side);
}

QPointF ColorWheel::wheelCenter() const {
    QRectF r = wheelRect();
    return r.center();
}

float ColorWheel::wheelRadius() const {
    return wheelRect().width() / 2.0f;
}

void ColorWheel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF rect = wheelRect();
    QPointF c = rect.center();
    float radius = rect.width() / 2.0f;
    if (radius < 4) return;

    /* Hue ring (conical gradient — full saturation around the rim). */
    QConicalGradient hue(c, 0);
    hue.setColorAt(0.000, QColor::fromHsvF(0.000, 1, 1));
    hue.setColorAt(0.167, QColor::fromHsvF(0.167, 1, 1));
    hue.setColorAt(0.333, QColor::fromHsvF(0.333, 1, 1));
    hue.setColorAt(0.500, QColor::fromHsvF(0.500, 1, 1));
    hue.setColorAt(0.667, QColor::fromHsvF(0.667, 1, 1));
    hue.setColorAt(0.833, QColor::fromHsvF(0.833, 1, 1));
    hue.setColorAt(1.000, QColor::fromHsvF(0.000, 1, 1));

    /* White centre fading to full saturation at rim. */
    QRadialGradient sat(c, radius);
    sat.setColorAt(0.0, QColor(255, 255, 255, 255));
    sat.setColorAt(1.0, QColor(255, 255, 255, 0));

    p.setPen(Qt::NoPen);
    p.setBrush(hue);
    p.drawEllipse(c, radius, radius);
    p.setBrush(sat);
    p.drawEllipse(c, radius, radius);

    /* Border. */
    p.setPen(QPen(QColor(40, 40, 40), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, radius, radius);

    /* Crosshair at centre. */
    p.setPen(QPen(QColor(0, 0, 0, 90), 1));
    p.drawLine(QPointF(c.x() - 6, c.y()), QPointF(c.x() + 6, c.y()));
    p.drawLine(QPointF(c.x(), c.y() - 6), QPointF(c.x(), c.y() + 6));

    /* Dot. */
    QPointF dot(c.x() + m_x * radius, c.y() - m_y * radius);
    p.setPen(QPen(Qt::black, 1.5));
    p.setBrush(Qt::white);
    p.drawEllipse(dot, 5, 5);
}

void ColorWheel::setDotFromPos(const QPointF &pos) {
    QPointF c = wheelCenter();
    float r = wheelRadius();
    if (r < 1) return;
    float nx = (pos.x() - c.x()) / r;
    float ny = -(pos.y() - c.y()) / r;
    float r2 = nx*nx + ny*ny;
    if (r2 > 1.0f) { float rn = std::sqrt(r2); nx /= rn; ny /= rn; }
    m_x = nx; m_y = ny;
    update();
    emit valuesChanged(m_x, m_y, m_lum);
}

void ColorWheel::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;
    QPointF c = wheelCenter();
    float r = wheelRadius();
    float dx = e->position().x() - c.x();
    float dy = e->position().y() - c.y();
    if (dx*dx + dy*dy > r*r) return;
    m_dragging = true;
    setDotFromPos(e->position());
}

void ColorWheel::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging) return;
    setDotFromPos(e->position());
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) m_dragging = false;
}

void ColorWheel::mouseDoubleClickEvent(QMouseEvent *e) {
    QPointF c = wheelCenter();
    float r = wheelRadius();
    float dx = e->position().x() - c.x();
    float dy = e->position().y() - c.y();
    if (dx*dx + dy*dy > r*r) return;
    m_x = m_y = 0.0f;
    update();
    emit valuesChanged(m_x, m_y, m_lum);
}

void ColorWheel::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    update();
}
