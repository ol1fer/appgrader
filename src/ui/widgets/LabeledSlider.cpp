#include "LabeledSlider.h"
#include <QHBoxLayout>
#include <QDoubleSpinBox>

LabeledSlider::LabeledSlider(const QString &label, float min, float max,
                             float step, QWidget *parent)
    : QWidget(parent), m_min(min), m_max(max), m_step(step), m_default(0.0f)
{
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(4);

    auto *lbl = new QLabel(label, this);
    lbl->setFixedWidth(86);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lay->addWidget(lbl);

    m_resetBtn = new QToolButton(this);
    m_resetBtn->setText(QStringLiteral("↺"));   /* ↺ — anticlockwise open circle arrow */
    m_resetBtn->setToolTip("Reset to default (or double-click slider)");
    m_resetBtn->setFixedSize(22, 22);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setFocusPolicy(Qt::NoFocus);
    lay->addWidget(m_resetBtn);

    m_slider = new ResettableSlider(Qt::Horizontal, this);
    m_slider->setRange(0, 1000);
    m_slider->setValue(toSlider(0.0f));
    lay->addWidget(m_slider, 1);

    m_spin = new QDoubleSpinBox(this);
    m_spin->setRange(min, max);
    m_spin->setSingleStep(step);
    m_spin->setDecimals(2);
    m_spin->setFixedWidth(68);
    m_spin->setValue(0.0);
    m_spin->setFocusPolicy(Qt::ClickFocus);
    lay->addWidget(m_spin);

    connect(m_slider, &QSlider::valueChanged, this, [this](int iv) {
        float v = fromSlider(iv);
        m_spin->blockSignals(true);
        m_spin->setValue(v);
        m_spin->blockSignals(false);
        emit valueChanged(v);
    });
    connect(m_spin, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        m_slider->blockSignals(true);
        m_slider->setValue(toSlider((float)v));
        m_slider->blockSignals(false);
        emit valueChanged((float)v);
    });
    connect(m_slider, &ResettableSlider::doubleClicked,
            this, &LabeledSlider::reset);
    connect(m_resetBtn, &QToolButton::clicked,
            this, &LabeledSlider::reset);
}

int LabeledSlider::toSlider(float v) const {
    return (int)(((v - m_min) / (m_max - m_min)) * 1000.0f + 0.5f);
}

float LabeledSlider::fromSlider(int iv) const {
    return m_min + (m_max - m_min) * (iv / 1000.0f);
}

float LabeledSlider::value() const { return (float)m_spin->value(); }

void LabeledSlider::setValue(float v, bool silent) {
    if (silent) {
        m_slider->blockSignals(true);
        m_spin->blockSignals(true);
    }
    m_slider->setValue(toSlider(v));
    m_spin->setValue(v);
    if (silent) {
        m_slider->blockSignals(false);
        m_spin->blockSignals(false);
    }
}

void LabeledSlider::setDefaultValue(float v) {
    m_default = v;
    setValue(v, true);
}

void LabeledSlider::reset() { setValue(m_default); }
