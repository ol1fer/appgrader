#include "LutPanel.h"
#include "ui/widgets/LabeledSlider.h"
#include "config/settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QLabel>
#include <QUrl>
#include <algorithm>
#include <cstdlib>

LutPanel::LutPanel(QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(8, 8, 8, 8);
    lay->setSpacing(6);

    /* "Currently applied LUT: <name>" — surfaces what's actually being
     * baked into the LUT, separately from the list selection. Without
     * this label the user couldn't tell which LUT was active after
     * clicking around the list (highlight state was overloaded). */
    m_activeLabel = new QLabel(this);
    m_activeLabel->setTextFormat(Qt::RichText);
    lay->addWidget(m_activeLabel);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    lay->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout();
    auto *applyBtn  = new QPushButton("Apply Selected", this);
    auto *clearBtn  = new QPushButton("Clear", this);
    auto *importBtn = new QPushButton("Import .cube", this);
    auto *folderBtn = new QPushButton("Open Folder", this);
    auto *refreshBtn = new QPushButton("Refresh", this);
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    btnRow->addWidget(importBtn);
    btnRow->addWidget(folderBtn);
    btnRow->addWidget(refreshBtn);
    lay->addLayout(btnRow);

    m_intensity = new LabeledSlider("Intensity", 0.0f, 1.0f, 0.01f, this);
    m_intensity->setValue(1.0f, true);
    lay->addWidget(m_intensity);

    connect(applyBtn,   &QPushButton::clicked, this, &LutPanel::onApply);
    connect(clearBtn,   &QPushButton::clicked, this, &LutPanel::onClear);
    connect(importBtn,  &QPushButton::clicked, this, &LutPanel::onImport);
    connect(folderBtn,  &QPushButton::clicked, this, &LutPanel::onOpenFolder);
    connect(refreshBtn, &QPushButton::clicked, this, &LutPanel::onRefresh);
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *){ onApply(); });
    connect(m_intensity, &LabeledSlider::valueChanged, this, [this](float){ emit lutChanged(); });

    setActive(QString());
    refreshList();
}

void LutPanel::refreshList() {
    QString selected = m_list->currentItem() ? m_list->currentItem()->text() : QString();
    m_list->clear();

    /* Recursive scan so users can drop LUT packs as nested folders
     * (e.g. lutsDir/FUJIFILMLUTS/FUJI400.CUBE). Display name is the
     * relative path; absolute path is stashed in Qt::UserRole for Apply. */
    QDir root(AppPaths::lutsDir());
    QStringList entries;
    QDirIterator it(root.absolutePath(),
                    {"*.cube", "*.CUBE"},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString abs = it.next();
        const QString rel = root.relativeFilePath(abs);
        /* Cap recursion at 10 directories deep — guards against pathological
         * symlink loops or absurdly nested packs. */
        if (rel.count('/') > 10) continue;
        entries << rel;
    }
    std::sort(entries.begin(), entries.end(),
              [](const QString &a, const QString &b) {
                  return a.compare(b, Qt::CaseInsensitive) < 0;
              });
    for (const QString &rel : entries) {
        auto *item = new QListWidgetItem(rel);
        item->setData(Qt::UserRole, root.absoluteFilePath(rel));
        m_list->addItem(item);
    }

    /* Restore selection. */
    for (int i = 0; i < m_list->count(); i++) {
        if (m_list->item(i)->text() == selected) {
            m_list->setCurrentRow(i);
            break;
        }
    }
    /* If the active LUT got deleted on disk, drop it. The active label
     * shouldn't claim a file that no longer exists. */
    if (!m_activeName.isEmpty()) {
        bool stillThere = false;
        for (int i = 0; i < m_list->count(); i++) {
            if (m_list->item(i)->text() == m_activeName) {
                stillThere = true;
                break;
            }
        }
        if (!stillThere) onClear();
    }
}

void LutPanel::onRefresh() { refreshList(); }

void LutPanel::onOpenFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(AppPaths::lutsDir()));
}

void LutPanel::onImport() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Import LUT files", QDir::homePath(), "LUT Files (*.cube *.CUBE)");
    for (const QString &src : files) {
        QFileInfo fi(src);
        QString dst = AppPaths::lutsDir() + "/" + fi.fileName();
        if (!QFile::exists(dst))
            QFile::copy(src, dst);
    }
    refreshList();
}

void LutPanel::onApply() {
    auto *item = m_list->currentItem();
    if (!item) return;
    const QString name = item->text();
    const QString path = item->data(Qt::UserRole).toString();
    if (!loadCube(path)) {
        QMessageBox::warning(this, "AppGrader", "Failed to load LUT: " + name);
        return;
    }
    setActive(name);
    emit lutChanged();
}

void LutPanel::onClear() {
    if (m_lutData) { free(m_lutData); m_lutData = nullptr; }
    m_lutSize = 0;
    m_loaded  = false;
    /* Selection is intentionally preserved so the user can re-Apply the
     * same LUT in one click. The label is the source of truth for
     * "what's actually applied", not the row highlight. */
    setActive(QString());
    emit lutChanged();
}

bool LutPanel::loadCube(const QString &path) {
    CubeLut cube;
    if (cube_lut_load(path.toLocal8Bit().constData(), &cube) != 0)
        return false;

    if (m_lutData) free(m_lutData);
    int total = cube.size * cube.size * cube.size * 3;
    m_lutData = (float *)malloc(sizeof(float) * total);
    if (!m_lutData) { cube_lut_free(&cube); return false; }
    memcpy(m_lutData, cube.data, sizeof(float) * total);
    m_lutSize   = cube.size;
    m_loaded    = true;
    cube_lut_free(&cube);
    return true;
}

void LutPanel::setActive(const QString &name) {
    m_activeName = name;
    if (name.isEmpty())
        m_activeLabel->setText("<b>Currently applied LUT:</b> <i>(none)</i>");
    else
        m_activeLabel->setText(
            QStringLiteral("<b>Currently applied LUT:</b> %1").arg(name.toHtmlEscaped()));
}

float LutPanel::intensity() const { return m_intensity->value(); }

void LutPanel::loadParams(const ColorParams &p) {
    m_intensity->setValue(p.lut_intensity, true);
}

void LutPanel::saveParams(ColorParams *p) const {
    p->lut_intensity = m_intensity->value();
}
