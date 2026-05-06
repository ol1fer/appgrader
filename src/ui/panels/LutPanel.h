#pragma once
#include <QWidget>
#include <QString>
#include "core/color_math.h"
#include "core/cube_parser.h"
class LabeledSlider;
class QListWidget;
class QListWidgetItem;
class QLabel;

class LutPanel : public QWidget {
    Q_OBJECT
public:
    explicit LutPanel(QWidget *parent = nullptr);

    /* The baked LUT data from the currently loaded .cube, or nullptr if none. */
    const float *lutData() const  { return m_loaded ? m_lutData : nullptr; }
    int          lutSize() const  { return m_lutSize; }
    float        intensity() const;

    void loadParams(const ColorParams &p);
    void saveParams(ColorParams *p) const;

signals:
    void lutChanged();  /* fired when the active LUT or intensity changes */

private slots:
    void onImport();
    void onOpenFolder();
    void onRefresh();
    void onApply();
    void onClear();

private:
    void refreshList();
    bool loadCube(const QString &path);
    void setActive(const QString &name);   /* updates label + signal listeners */

    QListWidget   *m_list;
    LabeledSlider *m_intensity;
    QLabel        *m_activeLabel;
    bool           m_loaded = false;
    float         *m_lutData = nullptr;
    int            m_lutSize = 0;
    QString        m_activeName;           /* filename of currently applied LUT */
};
