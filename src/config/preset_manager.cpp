#include "preset_manager.h"
#include "settings.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QString safeFilename(const QString &name) {
    QString s = name;
    s.replace(QRegularExpression(R"([/\\:*?"<>|])"), "_");
    return s + ".json";
}

QString PresetManager::presetPath(const QString &name) {
    return AppPaths::presetsDir() + "/" + safeFilename(name);
}

QStringList PresetManager::listPresets() {
    QStringList names;
    QDir dir(AppPaths::presetsDir());
    for (const QString &f : dir.entryList({"*.json"}, QDir::Files, QDir::Name))
        names << f.left(f.length() - 5);
    return names;
}

bool PresetManager::savePreset(const QString &name, const ColorParams &p,
                               const SpatialParams *spatial) {
    QJsonObject o;
    o["exposure"]    = p.exposure;
    o["contrast"]    = p.contrast;
    o["brightness"]  = p.brightness;
    o["highlights"]  = p.highlights;
    o["shadows"]     = p.shadows;
    o["whites"]      = p.whites;
    o["blacks"]      = p.blacks;
    o["temperature"] = p.temperature;
    o["tint"]        = p.tint;
    o["saturation"]  = p.saturation;
    o["vibrance"]    = p.vibrance;
    o["hue"]         = p.hue;
    o["lut_intensity"] = p.lut_intensity;

    QJsonArray hsl_h, hsl_s, hsl_l;
    for (int i = 0; i < 8; i++) {
        hsl_h.append(p.hsl_hue[i]);
        hsl_s.append(p.hsl_sat[i]);
        hsl_l.append(p.hsl_lum[i]);
    }
    o["hsl_hue"] = hsl_h;
    o["hsl_sat"] = hsl_s;
    o["hsl_lum"] = hsl_l;

    QJsonObject wheels;
    wheels["lift_x"]   = p.lift_x;   wheels["lift_y"]   = p.lift_y;   wheels["lift_lum"]  = p.lift_lum;
    wheels["gamma_x"]  = p.gamma_x;  wheels["gamma_y"]  = p.gamma_y;  wheels["gamma_lum"] = p.gamma_lum;
    wheels["gain_x"]   = p.gain_x;   wheels["gain_y"]   = p.gain_y;   wheels["gain_lum"]  = p.gain_lum;
    o["wheels"] = wheels;

    /* spatial params live alongside the preset since the user edits them
     * in the same workflow even though they don't go through the LUT. */
    if (spatial) {
        QJsonObject sp;
        sp["vignette_amount"]   = spatial->vignette_amount;
        sp["vignette_distance"] = spatial->vignette_distance;
        sp["noise_amount"]      = spatial->noise_amount;
        sp["noise_color"]       = spatial->noise_color;
        sp["noise_scale"]       = spatial->noise_scale;
        sp["noise_seed"]        = spatial->noise_seed;
        sp["noise_static"]      = (int)spatial->noise_static;
        sp["sharpen_amount"]    = spatial->sharpen_amount;
        sp["clarity_amount"]    = spatial->clarity_amount;
        sp["bloom_amount"]      = spatial->bloom_amount;
        sp["bloom_threshold"]   = spatial->bloom_threshold;
        sp["bloom_radius"]      = spatial->bloom_radius;
        o["spatial"] = sp;
    }

    /* Curves */
    QJsonArray curves;
    for (int c = 0; c < 4; c++) {
        QJsonObject cv;
        cv["n"] = p.curve_num_points[c];
        QJsonArray cx, cy;
        for (int i = 0; i < p.curve_num_points[c]; i++) {
            cx.append(p.curve_x[c][i]);
            cy.append(p.curve_y[c][i]);
        }
        cv["x"] = cx; cv["y"] = cy;
        curves.append(cv);
    }
    o["curves"] = curves;

    QFile f(presetPath(name));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return true;
}

bool PresetManager::loadPreset(const QString &name, ColorParams *p,
                               SpatialParams *spatial) {
    QFile f(presetPath(name));
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();

    color_params_init(p);
    p->exposure    = (float)o["exposure"].toDouble();
    p->contrast    = (float)o["contrast"].toDouble();
    p->brightness  = (float)o["brightness"].toDouble();
    p->highlights  = (float)o["highlights"].toDouble();
    p->shadows     = (float)o["shadows"].toDouble();
    p->whites      = (float)o["whites"].toDouble();
    p->blacks      = (float)o["blacks"].toDouble();
    p->temperature = (float)o["temperature"].toDouble();
    p->tint        = (float)o["tint"].toDouble();
    p->saturation  = (float)o["saturation"].toDouble();
    p->vibrance    = (float)o["vibrance"].toDouble();
    p->hue         = (float)o["hue"].toDouble();
    p->lut_intensity = (float)o["lut_intensity"].toDouble(1.0);

    QJsonArray hsl_h = o["hsl_hue"].toArray();
    QJsonArray hsl_s = o["hsl_sat"].toArray();
    QJsonArray hsl_l = o["hsl_lum"].toArray();
    for (int i = 0; i < 8 && i < hsl_h.size(); i++) {
        p->hsl_hue[i] = (float)hsl_h[i].toDouble();
        p->hsl_sat[i] = (float)hsl_s[i].toDouble();
        p->hsl_lum[i] = (float)hsl_l[i].toDouble();
    }

    QJsonObject wheels = o["wheels"].toObject();
    p->lift_x    = (float)wheels["lift_x"].toDouble();
    p->lift_y    = (float)wheels["lift_y"].toDouble();
    p->lift_lum  = (float)wheels["lift_lum"].toDouble();
    p->gamma_x   = (float)wheels["gamma_x"].toDouble();
    p->gamma_y   = (float)wheels["gamma_y"].toDouble();
    p->gamma_lum = (float)wheels["gamma_lum"].toDouble();
    p->gain_x    = (float)wheels["gain_x"].toDouble();
    p->gain_y    = (float)wheels["gain_y"].toDouble();
    p->gain_lum  = (float)wheels["gain_lum"].toDouble();

    if (spatial) {
        spatial_params_init(spatial);
        if (o.contains("spatial")) {
            QJsonObject sp = o["spatial"].toObject();
            spatial->vignette_amount   = (float)sp["vignette_amount"].toDouble();
            spatial->vignette_distance = (float)sp["vignette_distance"].toDouble(0.5);
            spatial->noise_amount      = (float)sp["noise_amount"].toDouble();
            spatial->noise_color       = (float)sp["noise_color"].toDouble();
            spatial->noise_scale       = (float)sp["noise_scale"].toDouble();
            spatial->noise_seed        = (float)sp["noise_seed"].toDouble();
            spatial->noise_static      = sp["noise_static"].toInt(0);
            spatial->sharpen_amount    = (float)sp["sharpen_amount"].toDouble();
            spatial->clarity_amount    = (float)sp["clarity_amount"].toDouble();
            /* Defaults match spatial_params_init for old presets without these keys. */
            spatial->bloom_amount      = (float)sp["bloom_amount"].toDouble(0.0);
            spatial->bloom_threshold   = (float)sp["bloom_threshold"].toDouble(0.7);
            spatial->bloom_radius      = (float)sp["bloom_radius"].toDouble(0.5);
        }
    }

    QJsonArray curves = o["curves"].toArray();
    for (int c = 0; c < 4 && c < curves.size(); c++) {
        QJsonObject cv = curves[c].toObject();
        int n = cv["n"].toInt(2);
        p->curve_num_points[c] = n < 2 ? 2 : (n > 16 ? 16 : n);
        QJsonArray cx = cv["x"].toArray(), cy = cv["y"].toArray();
        for (int i = 0; i < p->curve_num_points[c]; i++) {
            p->curve_x[c][i] = (float)cx[i].toDouble();
            p->curve_y[c][i] = (float)cy[i].toDouble();
        }
    }
    return true;
}

bool PresetManager::deletePreset(const QString &name) {
    return QFile::remove(presetPath(name));
}
