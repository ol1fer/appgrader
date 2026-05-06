#pragma once

#include <kwin/effect/offscreeneffect.h>

#include <QObject>
#include <QString>
#include <QElapsedTimer>
#include <memory>
#include <unordered_set>

#include "core/spatial_params.h"

class QFileSystemWatcher;

namespace KWin {
class GLShader;
class GlLookUpTable3D;
class EffectWindow;
}

class AppGraderEffect : public KWin::OffscreenEffect
{
    Q_OBJECT
public:
    AppGraderEffect();
    ~AppGraderEffect() override;

    static bool supported();

    bool isActive() const override;
    int  requestedEffectChainPosition() const override;

    void drawWindow(const KWin::RenderTarget &renderTarget,
                    const KWin::RenderViewport &viewport,
                    KWin::EffectWindow *w,
                    int mask,
                    const KWin::Region &deviceRegion,
                    KWin::WindowPaintData &data) override;

    void prePaintScreen(KWin::ScreenPrePaintData &data,
                        std::chrono::milliseconds presentTime) override;

private Q_SLOTS:
    void onWindowAdded(KWin::EffectWindow *w);
    void onWindowDeleted(KWin::EffectWindow *w);
    void onWindowGeometryChanged(KWin::EffectWindow *w, const QRectF &oldGeometry);
    void onLutFileChanged();
    void onActiveTargetFileChanged();
    void onSpatialFileChanged();

private:
    void loadShader();
    void loadLutFromFile();
    void readActiveTarget();
    void readSpatialFromFile();
    bool shouldGrade(KWin::EffectWindow *w) const;
    void redirectWindow(KWin::EffectWindow *w);
    void redirectAllWindows();
    void reEvaluateRedirections();
    void watchGeometry(KWin::EffectWindow *w);
    void writeWindowsList();

    std::unique_ptr<KWin::GLShader>        m_shader;
    std::unique_ptr<KWin::GlLookUpTable3D> m_lut;
    int                                     m_lutSize = 0;

    std::unordered_set<KWin::EffectWindow *> m_windows;

    QFileSystemWatcher *m_watcher = nullptr;
    QString             m_lutPath;
    QString             m_activeTargetPath;
    QString             m_windowsListPath;
    QString             m_spatialPath;
    /* Selected target. Format: "output:<name>", "window:<uuid>", or empty
     * (grade all). For backwards compat, an unprefixed value is treated
     * as a monitor name. */
    QString             m_activeTarget;

    SpatialParams       m_spatial{};
    QElapsedTimer       m_clock;       /* monotonic source for shader time uniform */
};
