#include "ColorPanel.h"
#include "ui/widgets/LabeledSlider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

ColorPanel::ColorPanel(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(4);
    lay->setContentsMargins(8, 8, 8, 8);

    m_temp = new LabeledSlider("Temperature", -5.0f, 5.0f, 0.01f, this);
    m_tint = new LabeledSlider("Tint",        -5.0f, 5.0f, 0.01f, this);
    m_sat  = new LabeledSlider("Saturation",  -1.0f, 1.0f, 0.01f, this);
    m_vib  = new LabeledSlider("Vibrance",    -1.0f, 1.0f, 0.01f, this);
    m_hue  = new LabeledSlider("Hue",         -1.0f, 1.0f, 0.01f, this);

    lay->addWidget(m_temp);
    lay->addWidget(m_tint);
    lay->addWidget(m_sat);
    lay->addWidget(m_vib);
    lay->addWidget(m_hue);
    lay->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetBtn);
    lay->addLayout(btnRow);
    connect(resetBtn, &QPushButton::clicked, this, &ColorPanel::resetAll);

    auto sig = &LabeledSlider::valueChanged;
    connect(m_temp, sig, this, [this](float){ emit paramsChanged(); });
    connect(m_tint, sig, this, [this](float){ emit paramsChanged(); });
    connect(m_sat,  sig, this, [this](float){ emit paramsChanged(); });
    connect(m_vib,  sig, this, [this](float){ emit paramsChanged(); });
    connect(m_hue,  sig, this, [this](float){ emit paramsChanged(); });
}

void ColorPanel::loadParams(const ColorParams &p) {
    m_temp->setValue(p.temperature, true);
    m_tint->setValue(p.tint, true);
    m_sat->setValue(p.saturation, true);
    m_vib->setValue(p.vibrance, true);
    m_hue->setValue(p.hue, true);
}

void ColorPanel::saveParams(ColorParams *p) const {
    p->temperature = m_temp->value();
    p->tint        = m_tint->value();
    p->saturation  = m_sat->value();
    p->vibrance    = m_vib->value();
    p->hue         = m_hue->value();
}

void ColorPanel::resetAll() {
    m_temp->reset(); m_tint->reset(); m_sat->reset(); m_vib->reset(); m_hue->reset();
}
