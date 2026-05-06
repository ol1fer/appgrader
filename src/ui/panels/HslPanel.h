#pragma once
#include <QWidget>
#include <QTabWidget>
#include "core/color_math.h"
class LabeledSlider;

class HslPanel : public QWidget {
    Q_OBJECT
public:
    explicit HslPanel(QWidget *parent = nullptr);
    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    /* 8 colour ranges × 3 (H/S/L) */
    LabeledSlider *m_hue[8], *m_sat[8], *m_lum[8];
};
