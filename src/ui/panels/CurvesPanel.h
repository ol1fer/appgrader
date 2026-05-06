#pragma once
#include <QWidget>
#include "core/color_math.h"
class CurveEditor;

class CurvesPanel : public QWidget {
    Q_OBJECT
public:
    explicit CurvesPanel(QWidget *parent = nullptr);
    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    CurveEditor *m_curves[4]; /* 0=master, 1=R, 2=G, 3=B */
};
