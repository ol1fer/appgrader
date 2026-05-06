#include "HslPanel.h"
#include "ui/widgets/LabeledSlider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QScrollArea>

static const char *RANGE_NAMES[8] = {
    "Reds", "Oranges", "Yellows", "Greens", "Cyans", "Blues", "Purples", "Magentas"
};

HslPanel::HslPanel(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget(this);

    /* Create Hue / Sat / Lum sub-tabs */
    struct SubTab { const char *title; LabeledSlider **sliders; };
    SubTab subs[3] = { {"Hue", m_hue}, {"Saturation", m_sat}, {"Luminance", m_lum} };

    for (auto &sub : subs) {
        auto *w = new QWidget();
        auto *vl = new QVBoxLayout(w);
        vl->setSpacing(4);
        vl->setContentsMargins(8, 8, 8, 8);
        for (int i = 0; i < 8; i++) {
            sub.sliders[i] = new LabeledSlider(RANGE_NAMES[i], -1.0f, 1.0f, 0.01f, w);
            vl->addWidget(sub.sliders[i]);
            connect(sub.sliders[i], &LabeledSlider::valueChanged,
                    this, [this](float){ emit paramsChanged(); });
        }
        vl->addStretch();
        tabs->addTab(w, sub.title);
    }

    lay->addWidget(tabs);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetBtn);
    lay->addLayout(btnRow);
    connect(resetBtn, &QPushButton::clicked, this, &HslPanel::resetAll);
}

void HslPanel::loadParams(const ColorParams &p) {
    for (int i = 0; i < 8; i++) {
        m_hue[i]->setValue(p.hsl_hue[i], true);
        m_sat[i]->setValue(p.hsl_sat[i], true);
        m_lum[i]->setValue(p.hsl_lum[i], true);
    }
}

void HslPanel::saveParams(ColorParams *p) const {
    for (int i = 0; i < 8; i++) {
        p->hsl_hue[i] = m_hue[i]->value();
        p->hsl_sat[i] = m_sat[i]->value();
        p->hsl_lum[i] = m_lum[i]->value();
    }
}

void HslPanel::resetAll() {
    for (int i = 0; i < 8; i++) {
        m_hue[i]->reset(); m_sat[i]->reset(); m_lum[i]->reset();
    }
}
