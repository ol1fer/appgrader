#include "BasicPanel.h"
#include "ui/widgets/LabeledSlider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

BasicPanel::BasicPanel(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setSpacing(4);
    lay->setContentsMargins(8, 8, 8, 8);

    m_exposure    = new LabeledSlider("Exposure",    -3.0f,  3.0f, 0.05f, this);
    m_contrast    = new LabeledSlider("Contrast",    -1.0f,  1.0f, 0.01f, this);
    m_brightness  = new LabeledSlider("Brightness",  -1.0f,  1.0f, 0.01f, this);
    m_highlights  = new LabeledSlider("Highlights",  -1.0f,  1.0f, 0.01f, this);
    m_shadows     = new LabeledSlider("Shadows",     -1.0f,  1.0f, 0.01f, this);
    m_whites      = new LabeledSlider("Whites",      -1.0f,  1.0f, 0.01f, this);
    m_blacks      = new LabeledSlider("Blacks",      -1.0f,  1.0f, 0.01f, this);

    lay->addWidget(m_exposure);
    lay->addWidget(m_contrast);
    lay->addWidget(m_brightness);
    lay->addWidget(m_highlights);
    lay->addWidget(m_shadows);
    lay->addWidget(m_whites);
    lay->addWidget(m_blacks);
    lay->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetBtn);
    lay->addLayout(btnRow);
    connect(resetBtn, &QPushButton::clicked, this, &BasicPanel::resetAll);

    auto sig = &LabeledSlider::valueChanged;
    connect(m_exposure,   sig, this, [this](float){ emit paramsChanged(); });
    connect(m_contrast,   sig, this, [this](float){ emit paramsChanged(); });
    connect(m_brightness, sig, this, [this](float){ emit paramsChanged(); });
    connect(m_highlights, sig, this, [this](float){ emit paramsChanged(); });
    connect(m_shadows,    sig, this, [this](float){ emit paramsChanged(); });
    connect(m_whites,     sig, this, [this](float){ emit paramsChanged(); });
    connect(m_blacks,     sig, this, [this](float){ emit paramsChanged(); });
}

void BasicPanel::loadParams(const ColorParams &p) {
    m_exposure->setValue(p.exposure, true);
    m_contrast->setValue(p.contrast, true);
    m_brightness->setValue(p.brightness, true);
    m_highlights->setValue(p.highlights, true);
    m_shadows->setValue(p.shadows, true);
    m_whites->setValue(p.whites, true);
    m_blacks->setValue(p.blacks, true);
}

void BasicPanel::saveParams(ColorParams *p) const {
    p->exposure   = m_exposure->value();
    p->contrast   = m_contrast->value();
    p->brightness = m_brightness->value();
    p->highlights = m_highlights->value();
    p->shadows    = m_shadows->value();
    p->whites     = m_whites->value();
    p->blacks     = m_blacks->value();
}

void BasicPanel::resetAll() {
    m_exposure->reset(); m_contrast->reset(); m_brightness->reset();
    m_highlights->reset(); m_shadows->reset(); m_whites->reset(); m_blacks->reset();
}
