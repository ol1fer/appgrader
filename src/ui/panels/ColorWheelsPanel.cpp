#include "ColorWheelsPanel.h"
#include "ui/widgets/ColorWheel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

ColorWheelsPanel::ColorWheelsPanel(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *row = new QHBoxLayout();
    row->setSpacing(6);
    m_lift  = new ColorWheel("Shadows",    this);
    m_gamma = new ColorWheel("Midtones",   this);
    m_gain  = new ColorWheel("Highlights", this);
    row->addWidget(m_lift,  1);
    row->addWidget(m_gamma, 1);
    row->addWidget(m_gain,  1);
    root->addLayout(row, 1);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetBtn);
    root->addLayout(btnRow);
    connect(resetBtn, &QPushButton::clicked, this, &ColorWheelsPanel::resetAll);

    auto sig = &ColorWheel::valuesChanged;
    connect(m_lift,  sig, this, [this](float, float, float){ emit paramsChanged(); });
    connect(m_gamma, sig, this, [this](float, float, float){ emit paramsChanged(); });
    connect(m_gain,  sig, this, [this](float, float, float){ emit paramsChanged(); });
}

void ColorWheelsPanel::loadParams(const ColorParams &p) {
    m_lift ->setValues(p.lift_x,  p.lift_y,  p.lift_lum,  true);
    m_gamma->setValues(p.gamma_x, p.gamma_y, p.gamma_lum, true);
    m_gain ->setValues(p.gain_x,  p.gain_y,  p.gain_lum,  true);
}

void ColorWheelsPanel::saveParams(ColorParams *p) const {
    p->lift_x    = m_lift ->x();   p->lift_y    = m_lift ->y();   p->lift_lum  = m_lift ->lum();
    p->gamma_x   = m_gamma->x();   p->gamma_y   = m_gamma->y();   p->gamma_lum = m_gamma->lum();
    p->gain_x    = m_gain ->x();   p->gain_y    = m_gain ->y();   p->gain_lum  = m_gain ->lum();
}

void ColorWheelsPanel::resetAll() {
    m_lift->reset(); m_gamma->reset(); m_gain->reset();
}
