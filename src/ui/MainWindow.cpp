#include "MainWindow.h"
#include "panels/BasicPanel.h"
#include "panels/ColorPanel.h"
#include "panels/ColorWheelsPanel.h"
#include "panels/HslPanel.h"
#include "panels/CurvesPanel.h"
#include "panels/LutPanel.h"
#include "panels/EffectsPanel.h"
#include "config/preset_manager.h"
#include "config/settings.h"
#include "core/lut3d.h"
#include "core/spatial_params.h"
#include "ui/EffectSetupDialog.h"
#include "ui/KWinInstaller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QCloseEvent>
#include <QMenu>
#include <QApplication>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFrame>
#ifdef Q_OS_LINUX
#include <QDBusInterface>
#endif
#include <cstdlib>
#include <cstring>

static const int LUT_SIZE = 33;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_backend(DisplayBackend::create())
{
    color_params_init(&m_params);
    spatial_params_init(&m_spatial);
    m_bakedLut = lut3d_alloc_identity(LUT_SIZE);

    setWindowTitle("AppGrader");
    setMinimumSize(720, 852);
    resize(740, 852);

    buildUi();
    buildTray();

    /* Throttle LUT writes so slider drags don't generate 100+ file writes/sec.
     * The KWin effect repaints when active.lut is replaced, so flooding it
     * causes visible flicker. ~16ms == one frame at 60Hz. */
    m_applyTimer = new QTimer(this);
    m_applyTimer->setSingleShot(true);
    m_applyTimer->setInterval(16);
    connect(m_applyTimer, &QTimer::timeout, this, &MainWindow::doApplyToDisplay);

    AppPaths::ensureDirsExist();

    /* Watch the windows.list file the KWin effect writes so the Target
     * combo refreshes whenever a window opens or closes. The file
     * watcher loses the inode after atomic-rename writes, so we also
     * watch the parent dir as recovery — gated on mtime to avoid
     * re-firing on every active.lut/spatial.params write during slider
     * drags. */
    m_targetWatcher = new QFileSystemWatcher(this);
    const QString listPath = AppPaths::windowsListPath();
    if (QFile::exists(listPath))
        m_targetWatcher->addPath(listPath);
    m_targetWatcher->addPath(AppPaths::configDir());
    connect(m_targetWatcher, &QFileSystemWatcher::fileChanged, this,
            [this, listPath](const QString &p) {
        if (QFile::exists(p) && !m_targetWatcher->files().contains(p))
            m_targetWatcher->addPath(p);
        if (p == listPath) refreshTargets();
    });
    connect(m_targetWatcher, &QFileSystemWatcher::directoryChanged, this,
            [this, listPath]() {
        QFileInfo fi(listPath);
        if (!fi.exists()) return;
        if (fi.lastModified() == m_lastWindowsListMtime) return;
        if (!m_targetWatcher->files().contains(listPath))
            m_targetWatcher->addPath(listPath);
        refreshTargets();
    });

    refreshTargets();
    refreshPresets();
    restoreAppState();
    writeActiveTarget();   /* tell the effect which target is selected */
    loadKWinEffect();
    /* Banner reflects current state; the loadEffect call above is async so we
     * also re-check shortly after to hide the banner once KWin acknowledges. */
    refreshSetupBanner();
    QTimer::singleShot(500, this, &MainWindow::refreshSetupBanner);
    doApplyToDisplay();    /* initial identity-LUT write, no need to throttle */
}

MainWindow::~MainWindow() {
    lut3d_free(m_bakedLut);
}

void MainWindow::buildUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    /* ── Top bar ── */
    auto *topBar = new QHBoxLayout();
    topBar->setSpacing(6);

    topBar->addWidget(new QLabel("Target:", this));
    m_targetCombo = new QComboBox(this);
    m_targetCombo->setMinimumWidth(180);
    /* Keep the popup wide enough to read long window captions even when
     * the combo itself is narrow. */
    m_targetCombo->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    topBar->addWidget(m_targetCombo, 1);

    m_reapplyBtn = new QPushButton("Reapply", this);
    m_reapplyBtn->setToolTip(
        "Re-evaluate which windows are graded.\n"
        "Use this if a window dragged from another monitor isn't being graded,\n"
        "or if the targeted window's identity got out of sync.");
    topBar->addWidget(m_reapplyBtn);

    topBar->addSpacing(12);
    topBar->addWidget(new QLabel("Preset:", this));
    m_presetCombo = new QComboBox(this);
    m_presetCombo->setMinimumWidth(130);
    m_presetCombo->setEditable(false);
    topBar->addWidget(m_presetCombo, 1);

    m_savePresetBtn   = new QPushButton("Save", this);
    m_deletePresetBtn = new QPushButton("Del", this);
    m_savePresetBtn->setFixedWidth(48);
    m_deletePresetBtn->setFixedWidth(36);
    topBar->addWidget(m_savePresetBtn);
    topBar->addWidget(m_deletePresetBtn);

    root->addLayout(topBar);

    /* ── First-run banner (shown only when KWin effect isn't loaded) ── */
    m_setupBanner = new QFrame(this);
    m_setupBanner->setFrameShape(QFrame::StyledPanel);
    m_setupBanner->setStyleSheet(
        "QFrame { background: #fff5f5; border: 1px solid #ffc9c9; border-radius: 4px; }"
        "QLabel { background: transparent; color: #1a1a1a; border: none; padding: 0; }");
    auto *bannerLay = new QHBoxLayout(m_setupBanner);
    bannerLay->setContentsMargins(8, 6, 8, 6);
    auto *bannerLbl = new QLabel(
        "<b>KWin effect isn't loaded.</b> AppGrader can't apply colours until it is.",
        m_setupBanner);
    bannerLbl->setWordWrap(true);
    auto *bannerBtn = new QPushButton("Set up…", m_setupBanner);
    bannerLay->addWidget(bannerLbl, 1);
    bannerLay->addWidget(bannerBtn);
    connect(bannerBtn, &QPushButton::clicked, this, &MainWindow::onOpenEffectSetup);
    root->addWidget(m_setupBanner);
    m_setupBanner->hide();   /* shown by refreshSetupBanner() */

    /* ── Panels ── */
    auto *tabs = new QTabWidget(this);
    m_basicPanel  = new BasicPanel(this);
    m_colorPanel  = new ColorPanel(this);
    m_wheelsPanel = new ColorWheelsPanel(this);
    m_hslPanel    = new HslPanel(this);
    m_curvesPanel = new CurvesPanel(this);
    m_lutPanel     = new LutPanel(this);
    m_effectsPanel = new EffectsPanel(this);

    tabs->addTab(m_basicPanel,   "Light");
    tabs->addTab(m_colorPanel,   "Colour");
    tabs->addTab(m_wheelsPanel,  "Wheels");
    tabs->addTab(m_hslPanel,     "HSL");
    tabs->addTab(m_curvesPanel,  "Curves");
    tabs->addTab(m_effectsPanel, "Effects");
    tabs->addTab(m_lutPanel,     "LUT");
    root->addWidget(tabs, 1);

    /* ── Bottom bar ── */
    auto *botBar = new QHBoxLayout();
    m_bypassCheck     = new QCheckBox("Bypass", this);
    m_resetBtn        = new QPushButton("Reset All", this);
    m_refreshBtn      = new QPushButton("Refresh", this);
    m_configFolderBtn = new QPushButton("Config Folder", this);
    m_setupBtn        = new QPushButton("KWin Effect…", this);
    m_quitBtn         = new QPushButton("Quit", this);
    botBar->addWidget(m_bypassCheck);
    botBar->addStretch();
    botBar->addWidget(m_resetBtn);
    botBar->addWidget(m_refreshBtn);
    botBar->addWidget(m_configFolderBtn);
    botBar->addWidget(m_setupBtn);
    botBar->addWidget(m_quitBtn);
    root->addLayout(botBar);

    /* ── Connections ── */
    connect(m_basicPanel,  &BasicPanel::paramsChanged,        this, &MainWindow::onParamsChanged);
    connect(m_colorPanel,  &ColorPanel::paramsChanged,        this, &MainWindow::onParamsChanged);
    connect(m_wheelsPanel, &ColorWheelsPanel::paramsChanged,  this, &MainWindow::onParamsChanged);
    connect(m_hslPanel,    &HslPanel::paramsChanged,          this, &MainWindow::onParamsChanged);
    connect(m_curvesPanel, &CurvesPanel::paramsChanged,       this, &MainWindow::onParamsChanged);
    connect(m_lutPanel,    &LutPanel::lutChanged,             this, &MainWindow::onParamsChanged);
    connect(m_effectsPanel,&EffectsPanel::paramsChanged,      this, &MainWindow::onParamsChanged);

    connect(m_targetCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onTargetChanged);
    connect(m_reapplyBtn, &QPushButton::clicked, this, &MainWindow::onReapply);

    connect(m_savePresetBtn,   &QPushButton::clicked, this, &MainWindow::onSavePreset);
    connect(m_deletePresetBtn, &QPushButton::clicked, this, &MainWindow::onDeletePreset);
    connect(m_presetCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::onLoadPreset);

    connect(m_bypassCheck, &QCheckBox::toggled, this, &MainWindow::onBypassToggled);
    connect(m_resetBtn,    &QPushButton::clicked, this, &MainWindow::onReset);
    connect(m_refreshBtn,  &QPushButton::clicked, this, &MainWindow::onRefresh);
    connect(m_configFolderBtn, &QPushButton::clicked, this, &MainWindow::onOpenConfigFolder);
    connect(m_setupBtn,        &QPushButton::clicked, this, &MainWindow::onOpenEffectSetup);
    connect(m_quitBtn,         &QPushButton::clicked, this, &MainWindow::quitApp);
}

void MainWindow::onOpenEffectSetup() {
    EffectSetupDialog dlg(this);
    dlg.exec();
    refreshSetupBanner();
}

void MainWindow::refreshSetupBanner() {
    if (!m_setupBanner) return;
    m_setupBanner->setVisible(!KWinInstaller::isLoaded());
}

void MainWindow::quitApp() {
    saveAppState();
    lut3d_write_display_identity(LUT_SIZE,
        AppPaths::activeLutPath().toLocal8Bit().constData());
    SpatialParams identity; spatial_params_init(&identity);
    spatial_params_write(&identity,
        AppPaths::spatialParamsPath().toLocal8Bit().constData());
    qApp->quit();
}

void MainWindow::buildTray() {
    m_tray = new QSystemTrayIcon(QIcon::fromTheme("video-display"), this);
    auto *menu = new QMenu(this);
    menu->addAction("Show", this, &MainWindow::show);
    menu->addAction("Bypass", this, [this](){
        m_bypassCheck->setChecked(!m_bypassCheck->isChecked());
    });
    menu->addSeparator();
    menu->addAction("Quit", this, &MainWindow::quitApp);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip("AppGrader");
    m_tray->show();
    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

/* ── Target management ───────────────────────────────────────────── */

/* Populate the Target combo with monitors first, then currently-open
 * windows from windows.list. Each item carries its target string in
 * itemData (e.g. "output:DP-2", "window:<uuid>"); the visible text is
 * "MONITOR: <name>" for monitors and the window caption for windows.
 *
 * Selection is preserved by data, not by index, so a refresh triggered
 * by a window opening/closing doesn't snap the combo to the wrong item.
 * If the previously-selected target is gone (e.g. window closed) we
 * fall back to the first monitor — the watcher reload signals that
 * change, which trickles down to the effect via writeActiveTarget. */
void MainWindow::refreshTargets() {
    QSignalBlocker blocker(m_targetCombo);
    const QString prevData = m_targetCombo->currentData().toString();
    m_targetCombo->clear();

    for (const auto &mi : m_backend->listMonitors()) {
        const QString name = QString::fromStdString(mi.name);
        m_targetCombo->addItem(QStringLiteral("MONITOR: ") + name,
                               QStringLiteral("output:") + name);
    }

    QFile f(AppPaths::windowsListPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray raw = f.readAll();
        for (const QByteArray &line : raw.split('\n')) {
            if (line.isEmpty()) continue;
            const QList<QByteArray> cols = line.split('\t');
            if (cols.size() < 3) continue;
            const QString uuid = QString::fromUtf8(cols[0]);
            const QString cls  = QString::fromUtf8(cols[1]);
            QString cap        = QString::fromUtf8(cols[2]);
            if (cap.isEmpty()) cap = cls;
            if (uuid.isEmpty()) continue;
            m_targetCombo->addItem(cap, QStringLiteral("window:") + uuid);
        }
    }
    QFileInfo fi(AppPaths::windowsListPath());
    m_lastWindowsListMtime = fi.exists() ? fi.lastModified() : QDateTime();

    int idx = m_targetCombo->findData(prevData);
    if (idx < 0) idx = 0;
    m_targetCombo->setCurrentIndex(idx);

    /* If the previously-targeted window vanished, push the new target
     * to the effect so it stops trying to redirect a dead UUID. */
    if (m_targetCombo->currentData().toString() != prevData)
        writeActiveTarget();
}

void MainWindow::refreshPresets() {
    m_presetCombo->blockSignals(true);
    QString prev = m_presetCombo->currentText();
    m_presetCombo->clear();
    m_presetCombo->addItem("(none)");
    for (const QString &n : PresetManager::listPresets())
        m_presetCombo->addItem(n);
    int idx = m_presetCombo->findText(prev);
    m_presetCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_presetCombo->blockSignals(false);
}

/* ── Param collection ────────────────────────────────────────────── */

void MainWindow::collectParamsFromUi(ColorParams *p) {
    m_basicPanel->saveParams(p);
    m_colorPanel->saveParams(p);
    m_wheelsPanel->saveParams(p);
    m_hslPanel->saveParams(p);
    m_curvesPanel->saveParams(p);
    m_lutPanel->saveParams(p);
    m_effectsPanel->saveParams(&m_spatial);
}

void MainWindow::loadParamsToUi(const ColorParams &p) {
    m_basicPanel->loadParams(p);
    m_colorPanel->loadParams(p);
    m_wheelsPanel->loadParams(p);
    m_hslPanel->loadParams(p);
    m_curvesPanel->loadParams(p);
    m_lutPanel->loadParams(p);
    m_effectsPanel->loadParams(m_spatial);
}

/* ── KWin effect management ──────────────────────────────────────── */

void MainWindow::loadKWinEffect() {
#ifdef Q_OS_LINUX
    QDBusInterface kwin("org.kde.KWin", "/Effects", "org.kde.kwin.Effects",
                        QDBusConnection::sessionBus());
    if (kwin.isValid())
        kwin.asyncCall("loadEffect", "appgrader_lut");
#endif
}

/* ── Apply pipeline ──────────────────────────────────────────────── */

void MainWindow::onParamsChanged() {
    collectParamsFromUi(&m_params);
    applyToDisplay();
}

/* Throttled entry point. Updates always go through here so we don't
 * write the LUT file on every slider tick during a drag. */
void MainWindow::applyToDisplay() {
    if (!m_applyTimer->isActive())
        m_applyTimer->start();
}

/* Trailing-edge of the throttle timer: do the actual bake + atomic write. */
void MainWindow::doApplyToDisplay() {
    const QByteArray lutPath     = AppPaths::activeLutPath().toLocal8Bit();
    const QByteArray spatialPath = AppPaths::spatialParamsPath().toLocal8Bit();
    if (m_bypassed) {
        lut3d_write_display_identity(LUT_SIZE, lutPath.constData());
        SpatialParams identity; spatial_params_init(&identity);
        spatial_params_write(&identity, spatialPath.constData());
        return;
    }
    const float *extLut = m_lutPanel->lutData();
    int extSize         = m_lutPanel->lutSize();
    lut3d_bake(m_bakedLut, LUT_SIZE, &m_params, extLut, extSize);
    lut3d_write_display_lut(m_bakedLut, LUT_SIZE, lutPath.constData());
    spatial_params_write(&m_spatial, spatialPath.constData());
}

void MainWindow::writeActiveTarget() {
    const QString target = m_targetCombo->currentData().toString();
    QFile f(AppPaths::activeTargetPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    f.write(target.toUtf8());
}

/* ── Slots ───────────────────────────────────────────────────────── */

void MainWindow::onTargetChanged(int) {
    writeActiveTarget();
    applyToDisplay();
}

/* Manual fallback for the "window I dragged here isn't graded" case. The
 * KWin effect picks up screen changes automatically via a per-window
 * geometry hook, but if that ever misses something, rewriting the
 * active_target file forces the effect to re-evaluate every window. */
void MainWindow::onReapply() {
    writeActiveTarget();
    applyToDisplay();
}

void MainWindow::onSavePreset() {
    bool ok;
    QString name = QInputDialog::getText(
        this, "Save Preset", "Preset name:", QLineEdit::Normal,
        m_presetCombo->currentText() == "(none)" ? "" : m_presetCombo->currentText(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();
    collectParamsFromUi(&m_params);
    if (!PresetManager::savePreset(name, m_params, &m_spatial)) {
        QMessageBox::warning(this, "AppGrader", "Failed to save preset.");
        return;
    }
    refreshPresets();
    int idx = m_presetCombo->findText(name);
    if (idx >= 0) {
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(idx);
        m_presetCombo->blockSignals(false);
    }
}

void MainWindow::onLoadPreset(const QString &name) {
    if (name.isEmpty() || name == "(none)") return;
    ColorParams p;
    SpatialParams s;
    if (!PresetManager::loadPreset(name, &p, &s)) return;
    m_params  = p;
    m_spatial = s;
    loadParamsToUi(m_params);
    applyToDisplay();
}

void MainWindow::onDeletePreset() {
    QString name = m_presetCombo->currentText();
    if (name == "(none)") return;
    if (QMessageBox::question(this, "AppGrader", "Delete preset \"" + name + "\"?",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    PresetManager::deletePreset(name);
    refreshPresets();
}

void MainWindow::onReset() {
    color_params_init(&m_params);
    spatial_params_init(&m_spatial);
    loadParamsToUi(m_params);
    m_effectsPanel->resetAll();
    applyToDisplay();
}

void MainWindow::onBypassToggled(bool b) {
    m_bypassed = b;
    applyToDisplay();
}

void MainWindow::onRefresh() {
    refreshTargets();
    refreshPresets();
}

void MainWindow::onOpenConfigFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::configDir()));
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason r) {
    if (r == QSystemTrayIcon::Trigger)
        isVisible() ? hide() : show();
}

void MainWindow::closeEvent(QCloseEvent *e) {
    saveAppState();
    hide();
    e->ignore();
}

void MainWindow::showEvent(QShowEvent *) {
    applyToDisplay();
}

/* ── State persistence ───────────────────────────────────────────── */

/* Slider values are NOT saved — fresh defaults on every launch.
 * Use Save Preset for anything you want to keep. Window geometry and the
 * selected target are saved as ergonomic conveniences only. */

void MainWindow::saveAppState() {
    QSettings s("AppGrader", "AppGrader");
    s.setValue("target",   m_targetCombo->currentData().toString());
    s.setValue("geometry", saveGeometry());
}

void MainWindow::restoreAppState() {
    QSettings s("AppGrader", "AppGrader");
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());

    /* Restore previously-selected target (refreshTargets() must run
     * first). Window UUIDs are session-unique so we don't try to
     * restore window targets — the user re-picks the window on next
     * run. Monitor names are stable, so those round-trip cleanly. */
    QString savedTarget = s.value("target").toString();
    if (savedTarget.startsWith(QLatin1String("output:"))) {
        int idx = m_targetCombo->findData(savedTarget);
        if (idx >= 0) {
            QSignalBlocker blocker(m_targetCombo);
            m_targetCombo->setCurrentIndex(idx);
        }
    }
}
