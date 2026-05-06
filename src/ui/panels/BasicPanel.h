#pragma once
#include <QWidget>
#include "core/color_math.h"

class LabeledSlider;

class BasicPanel : public QWidget {
    Q_OBJECT
public:
    explicit BasicPanel(QWidget *parent = nullptr);
    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    LabeledSlider *m_exposure, *m_contrast, *m_brightness,
                  *m_highlights, *m_shadows, *m_whites, *m_blacks;
};
