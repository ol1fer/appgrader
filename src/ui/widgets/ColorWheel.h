#pragma once
#include <QWidget>
class ResettableSlider;
class QLabel;

/* A circular Lumetri-style colour wheel: hue ring with a draggable inner dot
 * for chroma offset (x,y in unit disk), plus a vertical luminance slider on
 * the right side. Title shown above the wheel. */
class ColorWheel : public QWidget {
    Q_OBJECT
public:
    explicit ColorWheel(const QString &title, QWidget *parent = nullptr);

    float x() const   { return m_x;   }
    float y() const   { return m_y;   }
    float lum() const { return m_lum; }

    void setValues(float x, float y, float lum, bool silent = false);
    void reset();

signals:
    void valuesChanged(float x, float y, float lum);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    void setDotFromPos(const QPointF &p);
    QRectF wheelRect() const;
    QPointF wheelCenter() const;
    float wheelRadius() const;

    float m_x = 0.0f, m_y = 0.0f, m_lum = 0.0f;
    QLabel           *m_title;
    ResettableSlider *m_lumSlider;
    QLabel           *m_lumValue;
    bool m_dragging = false;
};
