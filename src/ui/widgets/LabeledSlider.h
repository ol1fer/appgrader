#pragma once
#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QToolButton>

/* QSlider variant that emits doubleClicked() when the user double-clicks anywhere
 * on the slider track. LabeledSlider wires this to reset(). */
class ResettableSlider : public QSlider {
    Q_OBJECT
public:
    explicit ResettableSlider(Qt::Orientation o, QWidget *parent = nullptr)
        : QSlider(o, parent) {}
signals:
    void doubleClicked();
protected:
    void mouseDoubleClickEvent(QMouseEvent *) override { emit doubleClicked(); }
};

/* A horizontal slider with: label · reset-button · slider · spinbox.
 * Double-click on the slider OR clicking the reset button returns the value
 * to the configured default. */
class LabeledSlider : public QWidget {
    Q_OBJECT
public:
    explicit LabeledSlider(const QString &label, float min, float max,
                           float step = 0.01f, QWidget *parent = nullptr);

    float value() const;
    void  setValue(float v, bool silent = false);
    void  setDefaultValue(float v);   /* changes what reset() / double-click goes to */
    void  reset();

signals:
    void valueChanged(float v);

private:
    float m_min, m_max, m_step, m_default;
    ResettableSlider *m_slider;
    QDoubleSpinBox   *m_spin;
    QToolButton      *m_resetBtn;

    int   toSlider(float v) const;
    float fromSlider(int v) const;
};
