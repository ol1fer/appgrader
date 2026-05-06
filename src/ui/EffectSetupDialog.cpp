#include "EffectSetupDialog.h"
#include "KWinInstaller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>

EffectSetupDialog::EffectSetupDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("KWin Effect Setup");
    setMinimumWidth(620);
    buildUi();
    refreshStatus();
}

void EffectSetupDialog::buildUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    auto *intro = new QLabel(
        "AppGrader applies its colour grade through a KWin compositor effect. "
        "The effect must be installed and loaded before grading takes effect.", this);
    intro->setWordWrap(true);
    root->addWidget(intro);

    /* Wrap the label inside a styled QFrame so the frame sizes around the
     * label's wrapped content. Putting the style on the label directly
     * had Qt clipping the wrapped text outside the painted border. */
    auto *warnFrame = new QFrame(this);
    warnFrame->setFrameShape(QFrame::StyledPanel);
    warnFrame->setStyleSheet(
        "QFrame { background: #fff5f5; border: 1px solid #ffc9c9; "
        "border-radius: 4px; }"
        "QLabel { background: transparent; color: #1a1a1a; "
        "border: none; padding: 0; }");
    auto *warnLay = new QVBoxLayout(warnFrame);
    warnLay->setContentsMargins(10, 8, 10, 8);
    warnLay->setSpacing(0);
    auto *warn = new QLabel(warnFrame);
    warn->setText(
        "<span style='color:#c92a2a;'><b>Heads up:</b></span> loading or "
        "replacing a KWin effect can occasionally crash KWin. If it does, "
        "your apps keep running but the desktop, panel, and shortcuts may "
        "stop responding — log out and back in to recover. <b>Save your "
        "work first.</b><br>"
        "Install / Update needs root via pkexec; you'll see the system "
        "password prompt.");
    warn->setWordWrap(true);
    warn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    warnLay->addWidget(warn);
    root->addWidget(warnFrame);

    auto *grid = new QVBoxLayout();
    grid->setSpacing(2);
    m_statusLine = new QLabel(this);
    m_pathLine   = new QLabel(this);
    m_pathLine->setStyleSheet("color: #666;");
    m_pathLine->setWordWrap(true);
    grid->addWidget(m_statusLine);
    grid->addWidget(m_pathLine);
    root->addLayout(grid);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);
    m_installBtn = new QPushButton("Install / Update", this);
    m_loadBtn    = new QPushButton("Load",   this);
    m_unloadBtn  = new QPushButton("Unload", this);
    m_reloadBtn  = new QPushButton("Reload", this);
    btnRow->addWidget(m_installBtn);
    btnRow->addWidget(m_loadBtn);
    btnRow->addWidget(m_unloadBtn);
    btnRow->addWidget(m_reloadBtn);
    btnRow->addStretch();
    auto *closeBtn = new QPushButton("Close", this);
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    connect(m_installBtn, &QPushButton::clicked, this, &EffectSetupDialog::onInstall);
    connect(m_loadBtn,    &QPushButton::clicked, this, &EffectSetupDialog::onLoad);
    connect(m_unloadBtn,  &QPushButton::clicked, this, &EffectSetupDialog::onUnload);
    connect(m_reloadBtn,  &QPushButton::clicked, this, &EffectSetupDialog::onReload);
    connect(closeBtn,     &QPushButton::clicked, this, &QDialog::accept);
}

bool EffectSetupDialog::confirmCrashRisk() {
    auto r = QMessageBox::warning(this, "Continue?",
        "Loading or unloading the KWin effect can crash KWin. Save your work "
        "first.\n\nProceed?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return r == QMessageBox::Yes;
}

void EffectSetupDialog::refreshStatus() {
    using KWinInstaller::Status;
    Status s = KWinInstaller::currentStatus();
    m_statusLine->setText(QString("<b>Status:</b> %1").arg(KWinInstaller::statusLabel(s)));

    QString src = KWinInstaller::findSourceLibrary();
    QString dst = KWinInstaller::systemPluginPath();
    QString stale = KWinInstaller::systemFileIsStale()
        ? " &nbsp;<span style='color:#c92a2a;'><b>(installed copy is out of date)</b></span>"
        : QString();
    m_pathLine->setText(QString(
        "Source: %1<br>"
        "Target: %2%3")
        .arg(src.isEmpty() ? "<i>not found</i>" : src)
        .arg(dst, stale));

    bool loaded     = (s == Status::Loaded);
    bool installed  = (s != Status::NotInstalled && s != Status::Unknown);
    bool haveSource = !src.isEmpty();

    m_installBtn->setEnabled(haveSource);
    m_loadBtn->setEnabled(installed && !loaded);
    m_unloadBtn->setEnabled(loaded);
    m_reloadBtn->setEnabled(installed);
}

void EffectSetupDialog::onInstall() {
    if (!confirmCrashRisk()) return;
    QString err;
    if (!KWinInstaller::installToSystem(&err)) {
        QMessageBox::critical(this, "Install failed", err);
        refreshStatus();
        return;
    }
    /* After installing, attempt to load. */
    KWinInstaller::load(&err);
    refreshStatus();
    if (!err.isEmpty())
        QMessageBox::warning(this, "Load",
            "Effect copied but loadEffect failed:\n" + err);
}

void EffectSetupDialog::onLoad() {
    if (!confirmCrashRisk()) return;
    QString err;
    if (!KWinInstaller::load(&err))
        QMessageBox::critical(this, "Load failed", err);
    refreshStatus();
}

void EffectSetupDialog::onUnload() {
    QString err;
    if (!KWinInstaller::unload(&err))
        QMessageBox::critical(this, "Unload failed", err);
    refreshStatus();
}

void EffectSetupDialog::onReload() {
    if (!confirmCrashRisk()) return;
    QString err;
    if (!KWinInstaller::reload(&err))
        QMessageBox::critical(this, "Reload failed", err);
    refreshStatus();
}
