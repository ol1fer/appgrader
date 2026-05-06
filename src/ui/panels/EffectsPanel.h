#pragma once
#include <QWidget>
#include "core/spatial_params.h"
class LabeledSlider;
class QCheckBox;
class QGroupBox;

class EffectsPanel : public QWidget {
    Q_OBJECT
public:
    explicit EffectsPanel(QWidget *parent = nullptr);
    void loadParams(const SpatialParams &p);
    void saveParams(SpatialParams *p) const;
    void resetAll();
signals:
    void paramsChanged();
private:
    /* Each group is a checkable QGroupBox (defaults unchecked = effect off).
     * When unchecked, saveParams zeroes the relevant amount field so the
     * shader receives a no-op and Qt greys out the child sliders. */
    QGroupBox *m_vigGroup, *m_noiseGroup, *m_sharpenGroup, *m_clarityGroup, *m_bloomGroup;
    LabeledSlider *m_vigAmt, *m_vigDist;
    LabeledSlider *m_noiseAmt, *m_noiseColor, *m_noiseScale, *m_noiseSeed;
    QCheckBox     *m_noiseAnimated;
    LabeledSlider *m_sharpen;
    LabeledSlider *m_clarity;
    LabeledSlider *m_bloomAmt, *m_bloomThresh, *m_bloomRadius;
};
