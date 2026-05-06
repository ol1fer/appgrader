#pragma once
#include <QDialog>
class QLabel;
class QPushButton;

class EffectSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit EffectSetupDialog(QWidget *parent = nullptr);

private slots:
    void refreshStatus();
    void onInstall();
    void onLoad();
    void onUnload();
    void onReload();

private:
    void buildUi();
    bool confirmCrashRisk();

    QLabel      *m_statusLine;
    QLabel      *m_pathLine;
    QPushButton *m_installBtn;
    QPushButton *m_loadBtn;
    QPushButton *m_unloadBtn;
    QPushButton *m_reloadBtn;
};
