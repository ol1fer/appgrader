#pragma once
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QDateTime>
#include <memory>
#include "core/color_math.h"
#include "core/spatial_params.h"
#include "backend/display_backend.h"

class QComboBox;
class QFrame;
class QTabWidget;
class QPushButton;
class QCheckBox;
class QLabel;
class QFileSystemWatcher;
class BasicPanel;
class ColorPanel;
class ColorWheelsPanel;
class HslPanel;
class CurvesPanel;
class LutPanel;
class EffectsPanel;

class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *) override;
    void showEvent(QShowEvent *) override;

private slots:
    void onParamsChanged();
    void onTargetChanged(int index);
    void onReapply();
    void onSavePreset();
    void onLoadPreset(const QString &name);
    void onDeletePreset();
    void onReset();
    void onBypassToggled(bool bypassed);
    void onRefresh();
    void onOpenConfigFolder();
    void onTrayActivated(QSystemTrayIcon::ActivationReason);
    void onOpenEffectSetup();
    void quitApp();

private:
    void buildUi();
    void buildTray();
    void refreshTargets();           /* repopulate the Target combo with monitors + windows */
    void refreshPresets();
    void applyToDisplay();           /* schedules a write via the throttle timer */
    void doApplyToDisplay();         /* the real write — runs from the timer */
    void writeActiveTarget();        /* writes ~/.config/appgrader/active_target */
    void loadKWinEffect();
    void refreshSetupBanner();
    void loadParamsToUi(const ColorParams &p);
    void collectParamsFromUi(ColorParams *p);
    void saveAppState();
    void restoreAppState();

    /* Toolbar / header */
    QFrame    *m_setupBanner = nullptr;
    QComboBox *m_targetCombo;
    QPushButton *m_reapplyBtn;
    QComboBox *m_presetCombo;
    QPushButton *m_savePresetBtn, *m_deletePresetBtn;
    QCheckBox   *m_bypassCheck;
    QPushButton *m_resetBtn, *m_refreshBtn, *m_configFolderBtn, *m_setupBtn, *m_quitBtn;

    /* Panels */
    BasicPanel       *m_basicPanel;
    ColorPanel       *m_colorPanel;
    ColorWheelsPanel *m_wheelsPanel;
    HslPanel         *m_hslPanel;
    CurvesPanel      *m_curvesPanel;
    LutPanel         *m_lutPanel;
    EffectsPanel     *m_effectsPanel;

    /* State */
    ColorParams   m_params;
    SpatialParams m_spatial;
    bool m_bypassed = false;
    float *m_bakedLut = nullptr;
    QTimer *m_applyTimer = nullptr;  /* coalesces slider drags into ≤1 write per ~16ms */

    /* Backend */
    std::unique_ptr<DisplayBackend> m_backend;

    /* windows.list watcher (effect-written, app-read). */
    QFileSystemWatcher *m_targetWatcher = nullptr;
    QDateTime           m_lastWindowsListMtime;

    /* Tray */
    QSystemTrayIcon *m_tray;
};
