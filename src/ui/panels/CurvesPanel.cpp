#include "CurvesPanel.h"
#include "ui/widgets/CurveEditor.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QHBoxLayout>

CurvesPanel::CurvesPanel(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);

    auto *tabs = new QTabWidget(this);
    struct { const char *label; QColor color; } defs[4] = {
        {"Master", Qt::white}, {"Red", QColor(220,80,80)},
        {"Green", QColor(80,200,80)}, {"Blue", QColor(80,130,220)}
    };

    for (int i = 0; i < 4; i++) {
        auto *w = new QWidget();
        auto *vl = new QVBoxLayout(w);
        vl->setContentsMargins(4, 4, 4, 4);

        m_curves[i] = new CurveEditor(defs[i].color, w);
        vl->addWidget(m_curves[i], 1);

        auto *resetBtn = new QPushButton("Reset", w);
        resetBtn->setFixedHeight(24);
        vl->addWidget(resetBtn);

        connect(m_curves[i], &CurveEditor::curveChanged,
                this, [this](){ emit paramsChanged(); });
        connect(resetBtn, &QPushButton::clicked, this, [this, i](){
            m_curves[i]->setPoints({{0,0},{1,1}});
            emit paramsChanged();
        });

        tabs->addTab(w, defs[i].label);
    }
    lay->addWidget(tabs);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetAllBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetAllBtn);
    lay->addLayout(btnRow);
    connect(resetAllBtn, &QPushButton::clicked, this, [this]() {
        resetAll();
        emit paramsChanged();
    });
}

void CurvesPanel::loadParams(const ColorParams &p) {
    for (int c = 0; c < 4; c++)
        m_curves[c]->importFromParams(p.curve_x[c], p.curve_y[c], p.curve_num_points[c]);
}

void CurvesPanel::saveParams(ColorParams *p) const {
    for (int c = 0; c < 4; c++)
        m_curves[c]->exportToParams(p->curve_x[c], p->curve_y[c], &p->curve_num_points[c]);
}

void CurvesPanel::resetAll() {
    for (int c = 0; c < 4; c++)
        m_curves[c]->setPoints({{0,0},{1,1}});
}
