#pragma once
#include <QString>
#include <QStringList>
#include "core/color_math.h"
#include "core/spatial_params.h"

class PresetManager {
public:
    static QStringList listPresets();
    /* Spatial pointer is optional on save; if null, the preset is written
     * without spatial fields. On load, if the file has no spatial section,
     * *spatial is set to identity. */
    static bool savePreset(const QString &name, const ColorParams &params,
                           const SpatialParams *spatial = nullptr);
    static bool loadPreset(const QString &name, ColorParams *params,
                           SpatialParams *spatial = nullptr);
    static bool deletePreset(const QString &name);
    static QString presetPath(const QString &name);
};
