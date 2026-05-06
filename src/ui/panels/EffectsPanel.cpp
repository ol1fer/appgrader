#include "EffectsPanel.h"
#include "ui/widgets/LabeledSlider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <cmath>

static QGroupBox *group(const QString &title, QWidget *parent) {
    auto *g = new QGroupBox(title, parent);
    g->setCheckable(true);
    g->setChecked(false);   /* effects start disabled — user opts in */
    auto *l = new QVBoxLayout(g);
    l->setContentsMargins(8, 6, 8, 6);
    l->setSpacing(2);
    return g;
}

EffectsPanel::EffectsPanel(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    m_vigGroup = group("Vignette", this);
    m_vigAmt  = new LabeledSlider("Amount",   -1.0f, 1.0f, 0.01f, m_vigGroup);
    m_vigDist = new LabeledSlider("Distance",  0.0f, 1.0f, 0.01f, m_vigGroup);
    m_vigDist->setDefaultValue(0.5f);   /* matches spatial_params_init */
    m_vigGroup->layout()->addWidget(m_vigAmt);
    m_vigGroup->layout()->addWidget(m_vigDist);
    root->addWidget(m_vigGroup);

    m_noiseGroup = group("Noise", this);
    m_noiseAmt   = new LabeledSlider("Amount",     0.0f, 1.0f, 0.01f, m_noiseGroup);
    m_noiseColor = new LabeledSlider("Colour mix", 0.0f, 1.0f, 0.01f, m_noiseGroup);
    m_noiseScale = new LabeledSlider("Scale",      0.0f, 1.0f, 0.01f, m_noiseGroup);
    m_noiseSeed  = new LabeledSlider("Seed",       0.0f, 1.0f, 0.01f, m_noiseGroup);
    m_noiseAnimated = new QCheckBox("Animated", m_noiseGroup);
    m_noiseAnimated->setChecked(false);
    m_noiseGroup->layout()->addWidget(m_noiseAmt);
    m_noiseGroup->layout()->addWidget(m_noiseColor);
    m_noiseGroup->layout()->addWidget(m_noiseScale);
    m_noiseGroup->layout()->addWidget(m_noiseSeed);
    m_noiseGroup->layout()->addWidget(m_noiseAnimated);
    root->addWidget(m_noiseGroup);

    m_sharpenGroup = group("Sharpen", this);
    m_sharpen = new LabeledSlider("Amount", 0.0f, 1.0f, 0.01f, m_sharpenGroup);
    m_sharpenGroup->layout()->addWidget(m_sharpen);
    root->addWidget(m_sharpenGroup);

    m_clarityGroup = group("Clarity", this);
    m_clarity = new LabeledSlider("Amount", -1.0f, 1.0f, 0.01f, m_clarityGroup);
    m_clarityGroup->layout()->addWidget(m_clarity);
    root->addWidget(m_clarityGroup);

    m_bloomGroup = group("Bloom", this);
    m_bloomAmt    = new LabeledSlider("Amount",    0.0f, 1.0f, 0.01f, m_bloomGroup);
    m_bloomThresh = new LabeledSlider("Threshold", 0.0f, 1.0f, 0.01f, m_bloomGroup);
    m_bloomThresh->setDefaultValue(0.7f);   /* matches spatial_params_init */
    m_bloomRadius = new LabeledSlider("Radius",    0.0f, 1.0f, 0.01f, m_bloomGroup);
    m_bloomRadius->setDefaultValue(0.5f);   /* matches spatial_params_init */
    m_bloomGroup->layout()->addWidget(m_bloomAmt);
    m_bloomGroup->layout()->addWidget(m_bloomThresh);
    m_bloomGroup->layout()->addWidget(m_bloomRadius);
    root->addWidget(m_bloomGroup);

    root->addStretch();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *resetBtn = new QPushButton("Reset Tab", this);
    btnRow->addWidget(resetBtn);
    root->addLayout(btnRow);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        resetAll();
        emit paramsChanged();
    });

    auto sig = &LabeledSlider::valueChanged;
    auto fwd = [this](float){ emit paramsChanged(); };
    connect(m_vigAmt,     sig, this, fwd);
    connect(m_vigDist,    sig, this, fwd);
    connect(m_noiseAmt,   sig, this, fwd);
    connect(m_noiseColor, sig, this, fwd);
    connect(m_noiseScale, sig, this, fwd);
    connect(m_noiseSeed,  sig, this, fwd);
    connect(m_noiseAnimated, &QCheckBox::toggled, this,
            [this](bool){ emit paramsChanged(); });
    connect(m_sharpen,    sig, this, fwd);
    connect(m_clarity,    sig, this, fwd);
    connect(m_bloomAmt,    sig, this, fwd);
    connect(m_bloomThresh, sig, this, fwd);
    connect(m_bloomRadius, sig, this, fwd);

    auto grpFwd = [this](bool){ emit paramsChanged(); };
    connect(m_vigGroup,     &QGroupBox::toggled, this, grpFwd);
    connect(m_noiseGroup,   &QGroupBox::toggled, this, grpFwd);
    connect(m_sharpenGroup, &QGroupBox::toggled, this, grpFwd);
    connect(m_clarityGroup, &QGroupBox::toggled, this, grpFwd);
    connect(m_bloomGroup,   &QGroupBox::toggled, this, grpFwd);

    SpatialParams init;
    spatial_params_init(&init);
    loadParams(init);
}

void EffectsPanel::loadParams(const SpatialParams &p) {
    m_vigAmt    ->setValue(p.vignette_amount,   true);
    m_vigDist   ->setValue(p.vignette_distance, true);
    m_noiseAmt  ->setValue(p.noise_amount,      true);
    m_noiseColor->setValue(p.noise_color,       true);
    m_noiseScale->setValue(p.noise_scale,       true);
    m_noiseSeed ->setValue(p.noise_seed,        true);
    m_noiseAnimated->blockSignals(true);
    m_noiseAnimated->setChecked(!p.noise_static);
    m_noiseAnimated->blockSignals(false);
    m_sharpen   ->setValue(p.sharpen_amount,    true);
    m_clarity   ->setValue(p.clarity_amount,    true);
    m_bloomAmt   ->setValue(p.bloom_amount,    true);
    m_bloomThresh->setValue(p.bloom_threshold, true);
    m_bloomRadius->setValue(p.bloom_radius,    true);

    /* Old presets and the init defaults have no enable flag — infer it from
     * whether the relevant amount field is non-zero. Emits no extra signal
     * because we block during the toggle. */
    const float eps = 1e-4f;
    auto setGroup = [](QGroupBox *g, bool on) {
        g->blockSignals(true);
        g->setChecked(on);
        g->blockSignals(false);
    };
    setGroup(m_vigGroup,     std::fabs(p.vignette_amount) > eps);
    setGroup(m_noiseGroup,   p.noise_amount > eps);
    setGroup(m_sharpenGroup, p.sharpen_amount > eps);
    setGroup(m_clarityGroup, std::fabs(p.clarity_amount) > eps);
    setGroup(m_bloomGroup,   p.bloom_amount > eps);
}

void EffectsPanel::saveParams(SpatialParams *p) const {
    spatial_params_init(p);
    p->vignette_amount   = m_vigGroup->isChecked()     ? m_vigAmt    ->value() : 0.0f;
    p->vignette_distance = m_vigDist   ->value();
    p->noise_amount      = m_noiseGroup->isChecked()   ? m_noiseAmt  ->value() : 0.0f;
    p->noise_color       = m_noiseColor->value();
    p->noise_scale       = m_noiseScale->value();
    p->noise_seed        = m_noiseSeed ->value();
    p->noise_static      = m_noiseAnimated->isChecked() ? 0 : 1;
    p->sharpen_amount    = m_sharpenGroup->isChecked() ? m_sharpen   ->value() : 0.0f;
    p->clarity_amount    = m_clarityGroup->isChecked() ? m_clarity   ->value() : 0.0f;
    p->bloom_amount      = m_bloomGroup->isChecked()   ? m_bloomAmt  ->value() : 0.0f;
    p->bloom_threshold   = m_bloomThresh->value();
    p->bloom_radius      = m_bloomRadius->value();
}

void EffectsPanel::resetAll() {
    m_vigAmt->reset(); m_vigDist->reset();
    m_noiseAmt->reset(); m_noiseColor->reset(); m_noiseScale->reset();
    m_noiseSeed->reset();
    m_noiseAnimated->setChecked(false);
    m_sharpen->reset(); m_clarity->reset();
    m_bloomAmt->reset(); m_bloomThresh->reset(); m_bloomRadius->reset();

    /* Block signals so each setChecked doesn't fire its own paramsChanged —
     * the caller (Reset Tab button) emits a single one after this returns. */
    for (QGroupBox *g : { m_vigGroup, m_noiseGroup, m_sharpenGroup,
                          m_clarityGroup, m_bloomGroup }) {
        g->blockSignals(true);
        g->setChecked(false);
        g->blockSignals(false);
    }
}
