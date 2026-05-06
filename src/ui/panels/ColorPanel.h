#pragma once
#include <QWidget>
#include "core/color_math.h"
class LabeledSlider;

class ColorPanel : public QWidget {
    Q_OBJECT
public:
    explicit ColorPanel(QWidget *parent = nullptr);
    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    LabeledSlider *m_temp, *m_tint, *m_sat, *m_vib, *m_hue;
};
