#pragma once
#include <QWidget>
#include "core/color_math.h"
class ColorWheel;

class ColorWheelsPanel : public QWidget {
    Q_OBJECT
public:
    explicit ColorWheelsPanel(QWidget *parent = nullptr);
    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    ColorWheel *m_lift;
    ColorWheel *m_gamma;
    ColorWheel *m_gain;
};
