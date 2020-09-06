// samplv1widget.cpp
//
/****************************************************************************
   Copyright (C) 2012-2020, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "samplv1widget.h"
#include "samplv1_param.h"

#include "samplv1_sample.h"
#include "samplv1_sched.h"

#include "samplv1widget_config.h"
#include "samplv1widget_control.h"

#include "samplv1widget_keybd.h"

#include "samplv1_controls.h"
#include "samplv1_programs.h"

#include "ui_samplv1widget.h"

#include <QMessageBox>
#include <QDir>
#include <QTimer>

#include <QShowEvent>
#include <QHideEvent>

#include <math.h>

#include <random>


//-------------------------------------------------------------------------
// samplv1widget - impl.
//

// Constructor.
samplv1widget::samplv1widget ( QWidget *pParent )
	: QWidget(pParent), p_ui(new Ui::samplv1widget), m_ui(*p_ui)
{
	Q_INIT_RESOURCE(samplv1);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	// HACK: Dark themes grayed/disabled color group fix...
	QPalette pal;
	if (pal.base().color().value() < 0x7f) {
		const QColor& color = pal.window().color();
		const int iGroups = int(QPalette::Active | QPalette::Inactive) + 1;
		for (int i = 0; i < iGroups; ++i) {
			const QPalette::ColorGroup group = QPalette::ColorGroup(i);
			pal.setBrush(group, QPalette::Light,    color.lighter(150));
			pal.setBrush(group, QPalette::Midlight, color.lighter(120));
			pal.setBrush(group, QPalette::Dark,     color.darker(150));
			pal.setBrush(group, QPalette::Mid,      color.darker(120));
			pal.setBrush(group, QPalette::Shadow,   color.darker(200));
		}
		pal.setColor(QPalette::Disabled, QPalette::ButtonText, pal.mid().color());
		QWidget::setPalette(pal);
	}
#endif

	m_ui.setupUi(this);

	// Init sched notifier.
	m_sched_notifier = nullptr;

	// Init swapable params A/B to default.
	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i)
		m_params_ab[i] = samplv1_param::paramDefaultValue(samplv1::ParamIndex(i));

	// Some property temporary holder.
	m_iLoopFade = 1;

	// Start clean.
	m_iUpdate = 0;

	// Replicate the stacked/pages
	for (int iTab = 0; iTab < m_ui.StackedWidget->count(); ++iTab)
		m_ui.TabBar->addTab(m_ui.StackedWidget->widget(iTab)->windowTitle());

	// Offset/Loop range font.
	const QFont& font = m_ui.Gen1ReverseKnob->font();
	const QFont font2(font.family(), font.pointSize() - 1);
	m_ui.Gen1OctavesTextLabel->setFont(font2);
	m_ui.Gen1OctavesComboBox->setFont(font);
	m_ui.Gen1OffsetRangeLabel->setFont(font2);
	m_ui.Gen1OffsetStartSpinBox->setFont(font);
	m_ui.Gen1OffsetEndSpinBox->setFont(font);
	m_ui.Gen1LoopRangeLabel->setFont(font2);
	m_ui.Gen1LoopStartSpinBox->setFont(font);
	m_ui.Gen1LoopEndSpinBox->setFont(font);
	m_ui.Gen1LoopFadeCheckBox->setFont(font);
	m_ui.Gen1LoopFadeSpinBox->setFont(font2);

	const QFontMetrics fm(font);
	m_ui.Gen1OctavesComboBox->setMaximumWidth(76);
	m_ui.Gen1OctavesComboBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1OffsetStartSpinBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1OffsetEndSpinBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1LoopStartSpinBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1LoopEndSpinBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1LoopFadeSpinBox->setMaximumHeight(fm.height() + 6);

	m_ui.Gen1OffsetStartSpinBox->setAccelerated(true);
	m_ui.Gen1OffsetEndSpinBox->setAccelerated(true);
	m_ui.Gen1LoopStartSpinBox->setAccelerated(true);
	m_ui.Gen1LoopEndSpinBox->setAccelerated(true);
	m_ui.Gen1LoopFadeSpinBox->setAccelerated(true);

	m_ui.Gen1OffsetStartSpinBox->setMinimum(0);
	m_ui.Gen1OffsetEndSpinBox->setMinimum(0);
	m_ui.Gen1LoopStartSpinBox->setMinimum(0);
	m_ui.Gen1LoopEndSpinBox->setMinimum(0);
	m_ui.Gen1LoopFadeSpinBox->setMinimum(0);

	// Sample octave tables (±otabs).
	m_ui.Gen1OctavesComboBox->clear();
	m_ui.Gen1OctavesComboBox->addItem(tr("None"));
	m_ui.Gen1OctavesComboBox->addItem(tr("1 octave"));
	for (int otabs = 2; otabs <= 4; ++otabs)
		m_ui.Gen1OctavesComboBox->addItem(tr("%1 octaves").arg(otabs));

	// Note names.
	QStringList notes;
	for (int note = 0; note < 128; ++note)
		notes << samplv1_ui::noteName(note).remove(QRegularExpression("/\\S+"));

	m_ui.Gen1SampleKnob->setScale(1000.0f);
	m_ui.Gen1SampleKnob->insertItems(0, notes);

	// Swappable params A/B group.
	QButtonGroup *pSwapParamsGroup = new QButtonGroup(this);
	pSwapParamsGroup->addButton(m_ui.SwapParamsAButton);
	pSwapParamsGroup->addButton(m_ui.SwapParamsBButton);
	pSwapParamsGroup->setExclusive(true);
	m_ui.SwapParamsAButton->setChecked(true);

	// Wave shapes.
	QStringList shapes;
	shapes << tr("Pulse");
	shapes << tr("Saw");
	shapes << tr("Sine");
	shapes << tr("Rand");
	shapes << tr("Noise");

	m_ui.Lfo1ShapeKnob->insertItems(0, shapes);

	// Filter types.
	QStringList types;
	types << tr("LPF");
	types << tr("BPF");
	types << tr("HPF");
	types << tr("BRF");

	m_ui.Dcf1TypeKnob->insertItems(0, types);

	// Filter slopes.
	QStringList slopes;
	slopes << tr("12dB/oct");
	slopes << tr("24dB/oct");
	slopes << tr("Biquad");
	slopes << tr("Formant");

	m_ui.Dcf1SlopeKnob->insertItems(0, slopes);

	// Dynamic states.
	QStringList states;
	states << tr("Off");
	states << tr("On");
#if 0
	m_ui.Gen1ReverseKnob->insertItems(0, states);
	m_ui.Gen1OffsetKnob->insertItems(0, states);
	m_ui.Gen1LoopKnob->insertItems(0, states);

	m_ui.Lfo1SyncKnob->insertItems(0, states);

	m_ui.Dyn1CompressKnob->insertItems(0, states);
	m_ui.Dyn1LimiterKnob->insertItems(0, states);
#else
	m_ui.Lfo1SyncKnob->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
#endif
	// Special values
	const QString& sOff = states.first();
	m_ui.Gen1GlideKnob->setSpecialValueText(sOff);
	m_ui.Cho1WetKnob->setSpecialValueText(sOff);
	m_ui.Fla1WetKnob->setSpecialValueText(sOff);
	m_ui.Pha1WetKnob->setSpecialValueText(sOff);
	m_ui.Del1WetKnob->setSpecialValueText(sOff);
	m_ui.Rev1WetKnob->setSpecialValueText(sOff);

	const QString& sAuto = tr("Auto");
	m_ui.Gen1EnvTimeKnob->setSpecialValueText(sAuto);
	m_ui.Lfo1BpmKnob->setSpecialValueText(sAuto);
	m_ui.Del1BpmKnob->setSpecialValueText(sAuto);

	// Wave integer widths.
	m_ui.Lfo1WidthKnob->setDecimals(0);

	// GEN note limits.
	m_ui.Gen1SampleKnob->setMinimum(0.0f);
	m_ui.Gen1SampleKnob->setMaximum(127.0f);

	// GEN octave limits.
	m_ui.Gen1OctaveKnob->setMinimum(-4.0f);
	m_ui.Gen1OctaveKnob->setMaximum(+4.0f);

	// GEN tune limits.
	m_ui.Gen1TuningKnob->setMinimum(-1.0f);
	m_ui.Gen1TuningKnob->setMaximum(+1.0f);

	// DCF volume (env.amount) limits.
	m_ui.Dcf1EnvelopeKnob->setMinimum(-1.0f);
	m_ui.Dcf1EnvelopeKnob->setMaximum(+1.0f);

	// LFO parameter limits.
	m_ui.Lfo1BpmKnob->setScale(1.0f);
	m_ui.Lfo1BpmKnob->setMinimum(0.0f);
	m_ui.Lfo1BpmKnob->setMaximum(360.0f);
//	m_ui.Lfo1BpmKnob->setSingleStep(1.0f);
	m_ui.Lfo1SweepKnob->setMinimum(-1.0f);
	m_ui.Lfo1SweepKnob->setMaximum(+1.0f);
	m_ui.Lfo1CutoffKnob->setMinimum(-1.0f);
	m_ui.Lfo1CutoffKnob->setMaximum(+1.0f);
	m_ui.Lfo1ResoKnob->setMinimum(-1.0f);
	m_ui.Lfo1ResoKnob->setMaximum(+1.0f);
	m_ui.Lfo1PitchKnob->setMinimum(-1.0f);
	m_ui.Lfo1PitchKnob->setMaximum(+1.0f);
	m_ui.Lfo1PanningKnob->setMinimum(-1.0f);
	m_ui.Lfo1PanningKnob->setMaximum(+1.0f);
	m_ui.Lfo1VolumeKnob->setMinimum(-1.0f);
	m_ui.Lfo1VolumeKnob->setMaximum(+1.0f);

	// Channel filters
	QStringList channels;
	channels << tr("Omni");
	for (int iChannel = 0; iChannel < 16; ++iChannel)
		channels << QString::number(iChannel + 1);

	m_ui.Def1ChannelKnob->insertItems(0, channels);

	// Mono switches.
	QStringList modes;
	modes << tr("Poly");
	modes << tr("Mono");
	modes << tr("Legato");

	m_ui.Def1MonoKnob->insertItems(0, modes);

	// Output (stereo-)width limits.
	m_ui.Out1WidthKnob->setMinimum(-1.0f);
	m_ui.Out1WidthKnob->setMaximum(+1.0f);

	// Output (stereo-)panning limits.
	m_ui.Out1PanningKnob->setMinimum(-1.0f);
	m_ui.Out1PanningKnob->setMaximum(+1.0f);

	// Effects (delay BPM)
	m_ui.Del1BpmKnob->setScale(1.0f);
	m_ui.Del1BpmKnob->setMinimum(0.0f);
	m_ui.Del1BpmKnob->setMaximum(360.0f);
//	m_ui.Del1BpmKnob->setSingleStep(1.0f);

	// Reverb (stereo-)width limits.
	m_ui.Rev1WidthKnob->setMinimum(-1.0f);
	m_ui.Rev1WidthKnob->setMaximum(+1.0f);

	// GEN1
	setParamKnob(samplv1::GEN1_SAMPLE,  m_ui.Gen1SampleKnob);
	setParamKnob(samplv1::GEN1_REVERSE, m_ui.Gen1ReverseKnob);
	setParamKnob(samplv1::GEN1_OFFSET,  m_ui.Gen1OffsetKnob);
	setParamKnob(samplv1::GEN1_LOOP,    m_ui.Gen1LoopKnob);
	setParamKnob(samplv1::GEN1_OCTAVE,  m_ui.Gen1OctaveKnob);
	setParamKnob(samplv1::GEN1_TUNING,  m_ui.Gen1TuningKnob);
	setParamKnob(samplv1::GEN1_GLIDE,   m_ui.Gen1GlideKnob);
	setParamKnob(samplv1::GEN1_ENVTIME, m_ui.Gen1EnvTimeKnob);

	// DCF1
	setParamKnob(samplv1::DCF1_ENABLED,  m_ui.Dcf1GroupBox->param());
	setParamKnob(samplv1::DCF1_CUTOFF,   m_ui.Dcf1CutoffKnob);
	setParamKnob(samplv1::DCF1_RESO,     m_ui.Dcf1ResoKnob);
	setParamKnob(samplv1::DCF1_TYPE,     m_ui.Dcf1TypeKnob);
	setParamKnob(samplv1::DCF1_SLOPE,    m_ui.Dcf1SlopeKnob);
	setParamKnob(samplv1::DCF1_ENVELOPE, m_ui.Dcf1EnvelopeKnob);
	setParamKnob(samplv1::DCF1_ATTACK,   m_ui.Dcf1AttackKnob);
	setParamKnob(samplv1::DCF1_DECAY,    m_ui.Dcf1DecayKnob);
	setParamKnob(samplv1::DCF1_SUSTAIN,  m_ui.Dcf1SustainKnob);
	setParamKnob(samplv1::DCF1_RELEASE,  m_ui.Dcf1ReleaseKnob);

	QObject::connect(
		m_ui.Dcf1Filt, SIGNAL(cutoffChanged(float)),
		m_ui.Dcf1CutoffKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1CutoffKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setCutoff(float)));

	QObject::connect(
		m_ui.Dcf1Filt, SIGNAL(resoChanged(float)),
		m_ui.Dcf1ResoKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1ResoKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setReso(float)));

	QObject::connect(
		m_ui.Dcf1TypeKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setType(float)));
	QObject::connect(
		m_ui.Dcf1SlopeKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Filt, SLOT(setSlope(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(attackChanged(float)),
		m_ui.Dcf1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(decayChanged(float)),
		m_ui.Dcf1DecayKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1DecayKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setDecay(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(sustainChanged(float)),
		m_ui.Dcf1SustainKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1SustainKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setSustain(float)));

	QObject::connect(
		m_ui.Dcf1Env, SIGNAL(releaseChanged(float)),
		m_ui.Dcf1ReleaseKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dcf1ReleaseKnob, SIGNAL(valueChanged(float)),
		m_ui.Dcf1Env, SLOT(setRelease(float)));

	// LFO1
	setParamKnob(samplv1::LFO1_ENABLED, m_ui.Lfo1GroupBox->param());
	setParamKnob(samplv1::LFO1_SHAPE,   m_ui.Lfo1ShapeKnob);
	setParamKnob(samplv1::LFO1_WIDTH,   m_ui.Lfo1WidthKnob);
	setParamKnob(samplv1::LFO1_BPM,     m_ui.Lfo1BpmKnob);
	setParamKnob(samplv1::LFO1_RATE,    m_ui.Lfo1RateKnob);
	setParamKnob(samplv1::LFO1_SYNC,    m_ui.Lfo1SyncKnob);
	setParamKnob(samplv1::LFO1_PANNING, m_ui.Lfo1PanningKnob);
	setParamKnob(samplv1::LFO1_VOLUME,  m_ui.Lfo1VolumeKnob);
	setParamKnob(samplv1::LFO1_CUTOFF,  m_ui.Lfo1CutoffKnob);
	setParamKnob(samplv1::LFO1_RESO,    m_ui.Lfo1ResoKnob);
	setParamKnob(samplv1::LFO1_PITCH,   m_ui.Lfo1PitchKnob);
	setParamKnob(samplv1::LFO1_SWEEP,   m_ui.Lfo1SweepKnob);
	setParamKnob(samplv1::LFO1_ATTACK,  m_ui.Lfo1AttackKnob);
	setParamKnob(samplv1::LFO1_DECAY,   m_ui.Lfo1DecayKnob);
	setParamKnob(samplv1::LFO1_SUSTAIN, m_ui.Lfo1SustainKnob);
	setParamKnob(samplv1::LFO1_RELEASE, m_ui.Lfo1ReleaseKnob);

	QObject::connect(
		m_ui.Lfo1ShapeKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Wave, SLOT(setWaveShape(float)));
	QObject::connect(
		m_ui.Lfo1Wave, SIGNAL(waveShapeChanged(float)),
		m_ui.Lfo1ShapeKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1WidthKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Wave, SLOT(setWaveWidth(float)));
	QObject::connect(
		m_ui.Lfo1Wave, SIGNAL(waveWidthChanged(float)),
		m_ui.Lfo1WidthKnob, SLOT(setValue(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(attackChanged(float)),
		m_ui.Lfo1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(decayChanged(float)),
		m_ui.Lfo1DecayKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1DecayKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setDecay(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(sustainChanged(float)),
		m_ui.Lfo1SustainKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1SustainKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setSustain(float)));

	QObject::connect(
		m_ui.Lfo1Env, SIGNAL(releaseChanged(float)),
		m_ui.Lfo1ReleaseKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Lfo1ReleaseKnob, SIGNAL(valueChanged(float)),
		m_ui.Lfo1Env, SLOT(setRelease(float)));

	// DCA1
	setParamKnob(samplv1::DCA1_ENABLED, m_ui.Dca1GroupBox->param());
	setParamKnob(samplv1::DCA1_VOLUME,  m_ui.Dca1VolumeKnob);
	setParamKnob(samplv1::DCA1_ATTACK,  m_ui.Dca1AttackKnob);
	setParamKnob(samplv1::DCA1_DECAY,   m_ui.Dca1DecayKnob);
	setParamKnob(samplv1::DCA1_SUSTAIN, m_ui.Dca1SustainKnob);
	setParamKnob(samplv1::DCA1_RELEASE, m_ui.Dca1ReleaseKnob);

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(attackChanged(float)),
		m_ui.Dca1AttackKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1AttackKnob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setAttack(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(decayChanged(float)),
		m_ui.Dca1DecayKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1DecayKnob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setDecay(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(sustainChanged(float)),
		m_ui.Dca1SustainKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1SustainKnob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setSustain(float)));

	QObject::connect(
		m_ui.Dca1Env, SIGNAL(releaseChanged(float)),
		m_ui.Dca1ReleaseKnob, SLOT(setValue(float)));
	QObject::connect(
		m_ui.Dca1ReleaseKnob, SIGNAL(valueChanged(float)),
		m_ui.Dca1Env, SLOT(setRelease(float)));

	// DEF1
	setParamKnob(samplv1::DEF1_PITCHBEND, m_ui.Def1PitchbendKnob);
	setParamKnob(samplv1::DEF1_MODWHEEL,  m_ui.Def1ModwheelKnob);
	setParamKnob(samplv1::DEF1_PRESSURE,  m_ui.Def1PressureKnob);
	setParamKnob(samplv1::DEF1_VELOCITY,  m_ui.Def1VelocityKnob);
	setParamKnob(samplv1::DEF1_CHANNEL,   m_ui.Def1ChannelKnob);
	setParamKnob(samplv1::DEF1_MONO,      m_ui.Def1MonoKnob);

	// OUT1
	setParamKnob(samplv1::OUT1_WIDTH,   m_ui.Out1WidthKnob);
	setParamKnob(samplv1::OUT1_PANNING, m_ui.Out1PanningKnob);
	setParamKnob(samplv1::OUT1_FXSEND,  m_ui.Out1FxSendKnob);
	setParamKnob(samplv1::OUT1_VOLUME,  m_ui.Out1VolumeKnob);

    // added by scotty to try to support linnstruments need for bigger bend range
    m_ui.Def1PitchbendKnob->setMinimum(0.0f);
    m_ui.Def1PitchbendKnob->setMaximum(+4.0f);


	// Effects
	setParamKnob(samplv1::CHO1_WET,   m_ui.Cho1WetKnob);
	setParamKnob(samplv1::CHO1_DELAY, m_ui.Cho1DelayKnob);
	setParamKnob(samplv1::CHO1_FEEDB, m_ui.Cho1FeedbKnob);
	setParamKnob(samplv1::CHO1_RATE,  m_ui.Cho1RateKnob);
	setParamKnob(samplv1::CHO1_MOD,   m_ui.Cho1ModKnob);

	setParamKnob(samplv1::FLA1_WET,   m_ui.Fla1WetKnob);
	setParamKnob(samplv1::FLA1_DELAY, m_ui.Fla1DelayKnob);
	setParamKnob(samplv1::FLA1_FEEDB, m_ui.Fla1FeedbKnob);
	setParamKnob(samplv1::FLA1_DAFT,  m_ui.Fla1DaftKnob);

	setParamKnob(samplv1::PHA1_WET,   m_ui.Pha1WetKnob);
	setParamKnob(samplv1::PHA1_RATE,  m_ui.Pha1RateKnob);
	setParamKnob(samplv1::PHA1_FEEDB, m_ui.Pha1FeedbKnob);
	setParamKnob(samplv1::PHA1_DEPTH, m_ui.Pha1DepthKnob);
	setParamKnob(samplv1::PHA1_DAFT,  m_ui.Pha1DaftKnob);

	setParamKnob(samplv1::DEL1_WET,   m_ui.Del1WetKnob);
	setParamKnob(samplv1::DEL1_DELAY, m_ui.Del1DelayKnob);
	setParamKnob(samplv1::DEL1_FEEDB, m_ui.Del1FeedbKnob);
	setParamKnob(samplv1::DEL1_BPM,   m_ui.Del1BpmKnob);

	// Reverb
	setParamKnob(samplv1::REV1_WET,   m_ui.Rev1WetKnob);
	setParamKnob(samplv1::REV1_ROOM,  m_ui.Rev1RoomKnob);
	setParamKnob(samplv1::REV1_DAMP,  m_ui.Rev1DampKnob);
	setParamKnob(samplv1::REV1_FEEDB, m_ui.Rev1FeedbKnob);
	setParamKnob(samplv1::REV1_WIDTH, m_ui.Rev1WidthKnob);

	// Dynamics
	setParamKnob(samplv1::DYN1_COMPRESS, m_ui.Dyn1CompressKnob);
	setParamKnob(samplv1::DYN1_LIMITER,  m_ui.Dyn1LimiterKnob);

	// Make status-bar keyboard range active.
	m_ui.StatusBar->keybd()->setNoteRange(true);

	// Sample management...
	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(loadSampleFile(const QString&)),
		SLOT(loadSample(const QString&)));
	QObject::connect(m_ui.Gen1OctavesComboBox,
		SIGNAL(activated(int)),
		SLOT(octavesChanged(int)));

	// Preset management
	QObject::connect(m_ui.Preset,
		SIGNAL(newPresetFile()),
		SLOT(newPreset()));
	QObject::connect(m_ui.Preset,
		SIGNAL(loadPresetFile(const QString&)),
		SLOT(loadPreset(const QString&)));
	QObject::connect(m_ui.Preset,
		SIGNAL(savePresetFile(const QString&)),
		SLOT(savePreset(const QString&)));
	QObject::connect(m_ui.Preset,
		SIGNAL(resetPresetFile()),
		SLOT(resetParams()));

	// Common context menu policies...
	m_ui.Gen1Sample->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.Gen1OffsetStartSpinBox->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.Gen1OffsetEndSpinBox->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.Gen1LoopStartSpinBox->setContextMenuPolicy(Qt::CustomContextMenu);
	m_ui.Gen1LoopEndSpinBox->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(contextMenuRequest(const QPoint&)));

	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(offsetRangeChanged()),
		SLOT(offsetRangeChanged()));
	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(loopRangeChanged()),
		SLOT(loopRangeChanged()));

	QObject::connect(m_ui.Gen1OffsetStartSpinBox,
		SIGNAL(valueChanged(uint32_t)),
		SLOT(offsetStartChanged()));
	QObject::connect(m_ui.Gen1OffsetEndSpinBox,
		SIGNAL(valueChanged(uint32_t)),
		SLOT(offsetEndChanged()));
	QObject::connect(m_ui.Gen1LoopStartSpinBox,
		SIGNAL(valueChanged(uint32_t)),
		SLOT(loopStartChanged()));
	QObject::connect(m_ui.Gen1LoopEndSpinBox,
		SIGNAL(valueChanged(uint32_t)),
		SLOT(loopEndChanged()));
	QObject::connect(m_ui.Gen1LoopFadeCheckBox,
		SIGNAL(valueChanged(float)),
		SLOT(loopFadeChanged()));
	QObject::connect(m_ui.Gen1LoopFadeSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(loopFadeChanged()));
	QObject::connect(m_ui.Gen1LoopZeroCheckBox,
		SIGNAL(valueChanged(float)),
		SLOT(loopZeroChanged()));

	QObject::connect(m_ui.Gen1OffsetStartSpinBox,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(spinboxContextMenu(const QPoint&)));
	QObject::connect(m_ui.Gen1OffsetEndSpinBox,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(spinboxContextMenu(const QPoint&)));
	QObject::connect(m_ui.Gen1LoopStartSpinBox,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(spinboxContextMenu(const QPoint&)));
	QObject::connect(m_ui.Gen1LoopEndSpinBox,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(spinboxContextMenu(const QPoint&)));

	// Randomize params...
	QObject::connect(m_ui.RandomParamsButton,
		SIGNAL(clicked()),
		SLOT(randomParams()));

	// Swap params A/B
	QObject::connect(m_ui.SwapParamsAButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));
	QObject::connect(m_ui.SwapParamsBButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));

	// Randomize params...
	QObject::connect(m_ui.PanicButton,
		SIGNAL(clicked()),
		SLOT(panic()));
	
	// Direct stacked-page signal/slot
	QObject::connect(m_ui.TabBar, SIGNAL(currentChanged(int)),
		m_ui.StackedWidget, SLOT(setCurrentIndex(int)));

	// Direct status-bar keyboard input
	QObject::connect(m_ui.StatusBar->keybd(),
		SIGNAL(noteOnClicked(int, int)),
		SLOT(directNoteOn(int, int)));
	QObject::connect(m_ui.StatusBar->keybd(),
		SIGNAL(noteRangeChanged()),
		SLOT(noteRangeChanged()));

	// Menu actions
	QObject::connect(m_ui.helpConfigureAction,
		SIGNAL(triggered(bool)),
		SLOT(helpConfigure()));
	QObject::connect(m_ui.helpAboutAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAbout()));
	QObject::connect(m_ui.helpAboutQtAction,
		SIGNAL(triggered(bool)),
		SLOT(helpAboutQt()));

	// General knob/dial behavior init...
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig) {
		samplv1widget_dial::setDialMode(
			samplv1widget_dial::DialMode(pConfig->iKnobDialMode));
		samplv1widget_edit::setEditMode(
			samplv1widget_edit::EditMode(pConfig->iKnobEditMode));
		const samplv1widget_spinbox::Format format
			= samplv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
		m_ui.Gen1OffsetStartSpinBox->setFormat(format);
		m_ui.Gen1OffsetEndSpinBox->setFormat(format);
		m_ui.Gen1LoopStartSpinBox->setFormat(format);
		m_ui.Gen1LoopEndSpinBox->setFormat(format);
	}

	// Epilog.
	// QWidget::adjustSize();

	m_ui.StatusBar->showMessage(tr("Ready"), 5000);
	m_ui.StatusBar->modified(false);
	m_ui.Preset->setDirtyPreset(false);
}


// Destructor.
samplv1widget::~samplv1widget (void)
{
	if (m_sched_notifier)
		delete m_sched_notifier;

	delete p_ui;
}


// Open/close the scheduler/work notifier.
void samplv1widget::openSchedNotifier (void)
{
	if (m_sched_notifier)
		return;

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	m_sched_notifier = new samplv1widget_sched(pSamplUi->instance(), this);

	QObject::connect(m_sched_notifier,
		SIGNAL(notify(int, int)),
		SLOT(updateSchedNotify(int, int)));

	pSamplUi->midiInEnabled(true);
}


void samplv1widget::closeSchedNotifier (void)
{
	if (m_sched_notifier) {
		delete m_sched_notifier;
		m_sched_notifier = nullptr;
	}

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->midiInEnabled(false);
}


// Show/hide widget handlers.
void samplv1widget::showEvent ( QShowEvent *pShowEvent )
{
	QWidget::showEvent(pShowEvent);

	openSchedNotifier();
}


void samplv1widget::hideEvent ( QHideEvent *pHideEvent )
{
	closeSchedNotifier();

	QWidget::hideEvent(pHideEvent);
}


// Param kbob (widget) map accesors.
void samplv1widget::setParamKnob ( samplv1::ParamIndex index, samplv1widget_param *pParam )
{
	pParam->setDefaultValue(samplv1_param::paramDefaultValue(index));

	m_paramKnobs.insert(index, pParam);
	m_knobParams.insert(pParam, index);

	QObject::connect(pParam,
		SIGNAL(valueChanged(float)),
		SLOT(paramChanged(float)));

	pParam->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(pParam,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(paramContextMenu(const QPoint&)));
}

samplv1widget_param *samplv1widget::paramKnob ( samplv1::ParamIndex index ) const
{
	return m_paramKnobs.value(index, nullptr);
}


// Param port accessors.
void samplv1widget::setParamValue (
	samplv1::ParamIndex index, float fValue, bool bIter )
{
	++m_iUpdate;

	samplv1widget_param *pParam = paramKnob(index);
	if (pParam)
		pParam->setValue(fValue);

	updateParamEx(index, fValue, bIter);

	--m_iUpdate;
}

float samplv1widget::paramValue ( samplv1::ParamIndex index ) const
{
	float fValue = 0.0f;

	samplv1widget_param *pParam = paramKnob(index);
	if (pParam) {
		fValue = pParam->value();
	} else {
		samplv1_ui *pSamplUi = ui_instance();
		if (pSamplUi)
			fValue = pSamplUi->paramValue(index);
	}

	return fValue;
}


// Param knob (widget) slot.
void samplv1widget::paramChanged ( float fValue )
{
	if (m_iUpdate > 0)
		return;

	samplv1widget_param *pParam = qobject_cast<samplv1widget_param *> (sender());
	if (pParam) {
		const samplv1::ParamIndex index = m_knobParams.value(pParam);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_ui.StatusBar->showMessage(QString("%1: %2")
			.arg(pParam->toolTip())
			.arg(pParam->valueText()), 5000);
		updateDirtyPreset(true);
	}
}


// Update local tied widgets.
void samplv1widget::updateParamEx (
	samplv1::ParamIndex index, float fValue, bool bIter )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	++m_iUpdate;

	switch (index) {
	case samplv1::GEN1_SAMPLE:
		m_ui.StatusBar->keybd()->setNoteKey(int(fValue));
		break;
	case samplv1::GEN1_REVERSE:
		pSamplUi->setReverse(bool(fValue > 0.0f));
		if (!bIter) updateSample(pSamplUi->sample());
		break;
	case samplv1::GEN1_OFFSET:
		pSamplUi->setOffset(bool(fValue > 0.0f));
		if (!bIter) updateOffsetLoop(pSamplUi->sample());
		break;
	case samplv1::GEN1_LOOP:
		pSamplUi->setLoop(bool(fValue > 0.0f));
		if (!bIter) updateOffsetLoop(pSamplUi->sample());
		break;
	case samplv1::DCF1_ENABLED:
		if (m_ui.Lfo1GroupBox->isChecked()) {
			const bool bDcf1Enabled = (fValue > 0.5f);
			m_ui.Lfo1CutoffKnob->setEnabled(bDcf1Enabled);
			m_ui.Lfo1ResoKnob->setEnabled(bDcf1Enabled);
		}
		break;
	case samplv1::LFO1_ENABLED:
		if (fValue > 0.5f) {
			const bool bDcf1Enabled = m_ui.Dcf1GroupBox->isChecked();
			m_ui.Lfo1CutoffKnob->setEnabled(bDcf1Enabled);
			m_ui.Lfo1ResoKnob->setEnabled(bDcf1Enabled);
		}
		break;
	case samplv1::DCF1_SLOPE:
		if (m_ui.Dcf1GroupBox->isChecked())
			m_ui.Dcf1TypeKnob->setEnabled(int(fValue) != 3); // !Formant
		break;
	case samplv1::LFO1_SHAPE:
		m_ui.Lfo1Wave->setWaveShape(fValue);
		break;
	case samplv1::KEY1_LOW:
		m_ui.StatusBar->keybd()->setNoteLow(int(fValue));
		break;
	case samplv1::KEY1_HIGH:
		m_ui.StatusBar->keybd()->setNoteHigh(int(fValue));
		break;
	case samplv1::DEF1_VELOCITY: {
		const int vel = int(79.375f * fValue + 47.625f) & 0x7f;
		m_ui.StatusBar->keybd()->setVelocity(vel);
		break;
	}
	default:
		break;
	}

	--m_iUpdate;
}


// Update scheduled controllers param/knob widgets.
void samplv1widget::updateSchedParam ( samplv1::ParamIndex index, float fValue )
{
	++m_iUpdate;

	samplv1widget_param *pParam = paramKnob(index);
	if (pParam) {
		pParam->setValue(fValue);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_ui.StatusBar->showMessage(QString("%1: %2")
			.arg(pParam->toolTip())
			.arg(pParam->valueText()), 5000);
		updateDirtyPreset(true);
	}

	--m_iUpdate;
}


// Reset all param knobs to default values.
void samplv1widget::resetParams (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	pSamplUi->reset();

	resetSwapParams();

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		float fValue = samplv1_param::paramDefaultValue(index);
		samplv1widget_param *pParam = paramKnob(index);
		if (pParam && pParam->isDefaultValue())
			fValue = pParam->defaultValue();
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		m_params_ab[i] = fValue;
	}

	updateSample(pSamplUi->sample());

	m_ui.StatusBar->showMessage(tr("Reset preset"), 5000);
	updateDirtyPreset(false);
}


// Randomize params (partial).
void samplv1widget::randomParams (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	float p = 1.0f;

	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig)
		p = 0.01f * pConfig->fRandomizePercent;

	if (QMessageBox::warning(this,
		tr("Warning"),
		tr("About to randomize current parameter values:\n\n"
		"-/+ %1%.\n\n"
		"Are you sure?").arg(100.0f * p),
		QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
		return;

	std::default_random_engine re(::time(nullptr));

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		// Filter out some non-randomizable parameters!...
		if (index == samplv1::GEN1_SAMPLE   ||
			index == samplv1::GEN1_OFFSET   ||
			index == samplv1::GEN1_OFFSET_1 ||
			index == samplv1::GEN1_OFFSET_2 ||
			index == samplv1::GEN1_LOOP     ||
			index == samplv1::GEN1_LOOP_1   ||
			index == samplv1::GEN1_LOOP_2   ||
			index == samplv1::GEN1_OCTAVE   ||
			index == samplv1::GEN1_TUNING   ||
			index == samplv1::GEN1_ENVTIME  ||
			index == samplv1::DCF1_ENABLED  ||
			index == samplv1::LFO1_ENABLED  ||
			index == samplv1::DCA1_ENABLED) 
			continue;
		if (index >= samplv1::OUT1_WIDTH)
			break;
		samplv1widget_param *pParam = paramKnob(index);
		if (pParam) {
			std::normal_distribution<float> nd;
			const float q = p * (pParam->maximum() - pParam->minimum());
			float fValue = pParam->value();
			if (samplv1_param::paramFloat(index))
				fValue += 0.5f * q * nd(re);
			else
				fValue = std::round(fValue + q * nd(re));
			if (fValue < pParam->minimum())
				fValue = pParam->minimum();
			else
			if (fValue > pParam->maximum())
				fValue = pParam->maximum();
			setParamValue(index, fValue);
			updateParam(index, fValue);
		}
	}

	m_ui.StatusBar->showMessage(tr("Randomize"), 5000);
	updateDirtyPreset(true);
}


// Swap params A/B.
void samplv1widget::swapParams ( bool bOn )
{
	if (m_iUpdate > 0 || !bOn)
		return;

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::swapParams(%d)", int(bOn));
#endif

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		samplv1widget_param *pParam = paramKnob(index);
		if (pParam) {
			const float fOldValue = pParam->value();
			const float fNewValue = m_params_ab[i];
			setParamValue(index, fNewValue, true);
			updateParam(index, fNewValue);
			m_params_ab[i] = fOldValue;
		}
	}

	updateSample(pSamplUi->sample());

	const bool bSwapA = m_ui.SwapParamsAButton->isChecked();
	m_ui.StatusBar->showMessage(tr("Swap %1").arg(bSwapA ? 'A' : 'B'), 5000);
	updateDirtyPreset(true);
}


// Panic: all-notes/sound-off (reset).
void samplv1widget::panic (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->reset();
}


// Reset swap params A/B group.
void samplv1widget::resetSwapParams (void)
{
	++m_iUpdate;
	m_ui.SwapParamsAButton->setChecked(true);
	--m_iUpdate;
}


// Initialize param values.
void samplv1widget::updateParamValues (void)
{
	resetSwapParams();

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		const float fValue = pSamplUi->paramValue(index);
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		m_params_ab[i] = fValue;
	}

	updateSample(pSamplUi->sample());
}


// Reset all param default values.
void samplv1widget::resetParamValues (void)
{
	resetSwapParams();

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		const float fValue = samplv1_param::paramDefaultValue(index);
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		m_params_ab[i] = fValue;
	}

	updateSample(pSamplUi->sample());
}


// Reset all knob default values.
void samplv1widget::resetParamKnobs (void)
{
	m_ui.Gen1OctavesComboBox->setCurrentIndex(0);

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1widget_param *pParam = paramKnob(samplv1::ParamIndex(i));
		if (pParam)
			pParam->resetDefaultValue();
	}
}


// Preset init.
void samplv1widget::initPreset (void)
{
	m_ui.Preset->initPreset();
}


// Preset clear.
void samplv1widget::clearPreset (void)
{
	m_ui.Preset->clearPreset();
}


// Preset renewal.
void samplv1widget::newPreset (void)
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::newPreset()");
#endif

	clearSampleFile();

	resetParamKnobs();
	resetParamValues();

	m_ui.StatusBar->showMessage(tr("New preset"), 5000);
	updateDirtyPreset(false);

//	openSample();
}


// Preset file I/O slots.
void samplv1widget::loadPreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::loadPreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	clearSampleFile();

	resetParamKnobs();
	resetParamValues();

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->loadPreset(sFilename);

	updateLoadPreset(QFileInfo(sFilename).completeBaseName());
}


void samplv1widget::savePreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::savePreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->savePreset(sFilename);

	const QString& sPreset
		= QFileInfo(sFilename).completeBaseName();

	m_ui.StatusBar->showMessage(tr("Save preset: %1").arg(sPreset), 5000);
	updateDirtyPreset(false);
}


// Sample reset slot.
void samplv1widget::clearSample (void)
{
	clearSampleFile();

	m_ui.StatusBar->showMessage(tr("Clear sample"), 5000);
	updateDirtyPreset(true);
}


// Sample file loader slots.
void samplv1widget::loadSample ( const QString& sFilename )
{
	const QFileInfo info(sFilename);
	const int iOctaves
		= m_ui.Gen1OctavesComboBox->currentIndex();

	loadSampleFile(info.canonicalFilePath(), iOctaves);
}


void samplv1widget::octavesChanged ( int iOctaves )
{
	samplv1_sample *pSample = m_ui.Gen1Sample->sample();
	if (pSample == nullptr)
		return;

	loadSampleFile(pSample->filename(), iOctaves);
}


// Sample openner.
void samplv1widget::openSample (void)
{
	m_ui.Gen1Sample->openSample();
}


// Sample file reset.
void samplv1widget::clearSampleFile (void)
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::clearSampleFile()");
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->setSampleFile(nullptr, 0);

	updateSample(nullptr);
}


// Sample file loader.
void samplv1widget::loadSampleFile ( const QString& sFilename, int iOctaves )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::loadSampleFile(\"%s\", %d)", sFilename.toUtf8().constData(), iOctaves);
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		pSamplUi->setSampleFile(sFilename.toUtf8().constData(), iOctaves);
		updateSample(pSamplUi->sample());
	}

	m_ui.StatusBar->showMessage(
		tr("Load sample: %1 (%2)").arg(sFilename).arg(iOctaves), 5000);
	updateDirtyPreset(true);
}


// Sample updater (crude experimental stuff II).
void samplv1widget::updateSample ( samplv1_sample *pSample, bool bDirty )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (m_ui.Gen1Sample->instance() == nullptr)
		m_ui.Gen1Sample->setInstance(pSamplUi);

	m_ui.Gen1Sample->setSample(pSample);

	++m_iUpdate;
	if (pSample) {
		activateParamKnobs(pSample->filename() != nullptr);
		updateOffsetLoop(pSample);
		// Set current preset name if empty...
		if (pSample->filename() && m_ui.Preset->preset().isEmpty()) {
			m_ui.Preset->setPreset(
				QFileInfo(pSample->filename()).completeBaseName());
		}
	} else {
		activateParamKnobs(false);
		updateOffsetLoop(nullptr);
	}
	--m_iUpdate;

	if (pSample && bDirty)
		updateDirtyPreset(true);
}


// Sample playback (direct note-on/off).
void samplv1widget::playSample (void)
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::playSample()");
#endif

	m_ui.Gen1Sample->directNoteOn();
}


// Dirty close prompt,
bool samplv1widget::queryClose (void)
{
	return m_ui.Preset->queryPreset();
}


// Offset start change.
void samplv1widget::offsetStartChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iOffsetStart = m_ui.Gen1OffsetStartSpinBox->value();
		const uint32_t iOffsetEnd = pSamplUi->offsetEnd();
		pSamplUi->setOffsetRange(iOffsetStart, iOffsetEnd);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


// Offset end change.
void samplv1widget::offsetEndChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iOffsetStart = pSamplUi->offsetStart();
		const uint32_t iOffsetEnd = m_ui.Gen1OffsetEndSpinBox->value();
		pSamplUi->setOffsetRange(iOffsetStart, iOffsetEnd);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


// Loop start change.
void samplv1widget::loopStartChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iLoopStart = m_ui.Gen1LoopStartSpinBox->value();
		const uint32_t iLoopEnd = pSamplUi->loopEnd();
		pSamplUi->setLoopRange(iLoopStart, iLoopEnd);
		m_ui.Gen1Sample->setLoopStart(iLoopStart);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


// Loop end change.
void samplv1widget::loopEndChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iLoopStart = pSamplUi->loopStart();
		const uint32_t iLoopEnd = m_ui.Gen1LoopEndSpinBox->value();
		pSamplUi->setLoopRange(iLoopStart, iLoopEnd);
		m_ui.Gen1Sample->setLoopEnd(iLoopEnd);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


// Loop fade change.
void samplv1widget::loopFadeChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const bool bLoopFade = (m_ui.Gen1LoopFadeCheckBox->value() > 0.5f);
		const uint32_t iLoopFade = m_ui.Gen1LoopFadeSpinBox->value();
		if (bLoopFade && iLoopFade > 0) m_iLoopFade = iLoopFade;
		pSamplUi->setLoopFade(bLoopFade ? m_iLoopFade : 0);
		m_ui.StatusBar->showMessage(tr("Loop crossfade: %1")
			.arg(bLoopFade ? QString::number(iLoopFade) : tr("Off")), 5000);
		m_ui.Gen1LoopFadeSpinBox->setEnabled(bLoopFade);
		updateDirtyPreset(true);
	}
	--m_iUpdate;
}


// Loop zero change.
void samplv1widget::loopZeroChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iLoopStart = pSamplUi->loopStart();
		const uint32_t iLoopEnd = pSamplUi->loopEnd();;
		const bool bLoopZero = (m_ui.Gen1LoopZeroCheckBox->value() > 0.5f);
		pSamplUi->setLoopZero(bLoopZero);
		pSamplUi->setLoopRange(iLoopStart, iLoopEnd);
		m_ui.StatusBar->showMessage(tr("Loop zero-crossing: %1")
			.arg(bLoopZero ? tr("On") : tr("Off")), 5000);
		updateDirtyPreset(true);
	}
	--m_iUpdate;
}


// Offset points changed (from UI).
void samplv1widget::offsetRangeChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iOffsetStart = m_ui.Gen1Sample->offsetStart();
		const uint32_t iOffsetEnd   = m_ui.Gen1Sample->offsetEnd();
		pSamplUi->setOffsetRange(iOffsetStart, iOffsetEnd);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


void samplv1widget::loopRangeChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iLoopStart = m_ui.Gen1Sample->loopStart();
		const uint32_t iLoopEnd   = m_ui.Gen1Sample->loopEnd();
		pSamplUi->setLoopRange(iLoopStart, iLoopEnd);
		updateOffsetLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


// Update offset/loop range change status.
void samplv1widget::updateOffsetLoop ( samplv1_sample *pSample, bool bDirty )
{
	if (pSample && pSample->filename()) {
		const uint16_t iOctaves   = pSample->otabs();
		const bool     bOffset    = pSample->isOffset();
		const uint32_t iOffsetStart = pSample->offsetStart();
		const uint32_t iOffsetEnd = pSample->offsetEnd();
		const bool     bLoop      = pSample->isLoop();
		const uint32_t iLoopStart = pSample->loopStart();
		const uint32_t iLoopEnd   = pSample->loopEnd();
		const uint32_t iLoopFade  = pSample->loopCrossFade();
		const bool     bLoopZero  = pSample->isLoopZeroCrossing();
		const uint32_t nframes    = pSample->length();
		const float    srate      = pSample->sampleRate();
		m_ui.Gen1OctavesComboBox->setCurrentIndex(iOctaves);
		m_ui.Gen1OffsetRangeLabel->setEnabled(bOffset);
		m_ui.Gen1OffsetStartSpinBox->setSampleRate(srate);
		m_ui.Gen1OffsetStartSpinBox->setEnabled(bOffset);
		m_ui.Gen1OffsetStartSpinBox->setMinimum(0);
		m_ui.Gen1OffsetStartSpinBox->setMaximum(bLoop ? iLoopStart : iOffsetEnd);
		m_ui.Gen1OffsetStartSpinBox->setValue(iOffsetStart);
		m_ui.Gen1OffsetEndSpinBox->setSampleRate(srate);
		m_ui.Gen1OffsetEndSpinBox->setEnabled(bOffset);
		m_ui.Gen1OffsetEndSpinBox->setMinimum(bLoop ? iLoopEnd : iOffsetStart);
		m_ui.Gen1OffsetEndSpinBox->setMaximum(nframes);
		m_ui.Gen1OffsetEndSpinBox->setValue(iOffsetEnd);
		m_ui.Gen1LoopRangeLabel->setEnabled(bLoop);
		m_ui.Gen1LoopStartSpinBox->setSampleRate(srate);
		m_ui.Gen1LoopStartSpinBox->setEnabled(bLoop);
		m_ui.Gen1LoopStartSpinBox->setMinimum(iOffsetStart);
		m_ui.Gen1LoopStartSpinBox->setMaximum(bLoop ? iLoopEnd : iOffsetEnd);
		m_ui.Gen1LoopEndSpinBox->setSampleRate(srate);
		m_ui.Gen1LoopEndSpinBox->setEnabled(bLoop);
		m_ui.Gen1LoopEndSpinBox->setMinimum(bLoop ? iLoopStart : iOffsetStart);
		m_ui.Gen1LoopEndSpinBox->setMaximum(iOffsetEnd);
		m_ui.Gen1LoopStartSpinBox->setValue(iLoopStart);
		m_ui.Gen1LoopEndSpinBox->setValue(iLoopEnd);
		m_ui.Gen1LoopFadeCheckBox->setEnabled(bLoop);
		m_ui.Gen1LoopFadeCheckBox->setValue(iLoopFade > 0 ? 1.0f : 0.0f);
		m_ui.Gen1LoopFadeSpinBox->setEnabled(bLoop && iLoopFade > 0);
		m_ui.Gen1LoopFadeSpinBox->setMinimum(0);
		m_ui.Gen1LoopFadeSpinBox->setMaximum(
			qMin(iLoopStart, (iLoopEnd - iLoopStart) >> 1));
		if (iLoopFade > 0) m_iLoopFade = iLoopFade;
		m_ui.Gen1LoopFadeSpinBox->setValue(m_iLoopFade);
		m_ui.Gen1LoopZeroCheckBox->setValue(bLoopZero ? 1.0f : 0.0f);
		m_ui.Gen1LoopZeroCheckBox->setEnabled(bLoop);
		m_ui.Gen1Sample->setOffsetStart(iOffsetStart);
		m_ui.Gen1Sample->setOffsetEnd(iOffsetEnd);
		m_ui.Gen1Sample->setOffset(bOffset);
		m_ui.Gen1Sample->setLoopStart(iLoopStart);
		m_ui.Gen1Sample->setLoopEnd(iLoopEnd);
		m_ui.Gen1Sample->setLoop(bLoop);
		updateParam(samplv1::GEN1_OFFSET_1, float(iOffsetStart) / float(nframes));
		updateParam(samplv1::GEN1_OFFSET_2, float(iOffsetEnd) / float(nframes));
		updateParam(samplv1::GEN1_LOOP_1, float(iLoopStart) / float(nframes));
		updateParam(samplv1::GEN1_LOOP_2, float(iLoopEnd) / float(nframes));
		if (bDirty) {
			QString sMessage;
			if (bOffset) {
				sMessage.append(tr("Offset: %1 - %2")
					.arg(m_ui.Gen1Sample->textFromValue(iOffsetStart))
					.arg(m_ui.Gen1Sample->textFromValue(iOffsetEnd)));
			}
			if (!sMessage.isEmpty()) {
				sMessage.append(',');
				sMessage.append(' ');
			}
			if (bLoop) {
				sMessage.append(tr("Loop: %1 - %2")
					.arg(m_ui.Gen1Sample->textFromValue(iLoopStart))
					.arg(m_ui.Gen1Sample->textFromValue(iLoopEnd)));
			}
			if (!sMessage.isEmpty())
				m_ui.StatusBar->showMessage(sMessage, 5000);
			updateDirtyPreset(true);
		}
	} else {
		m_ui.Gen1OffsetRangeLabel->setEnabled(false);
		m_ui.Gen1OffsetStartSpinBox->setEnabled(false);
		m_ui.Gen1OffsetStartSpinBox->setMinimum(0);
		m_ui.Gen1OffsetStartSpinBox->setMaximum(0);
		m_ui.Gen1OffsetStartSpinBox->setValue(0);
		m_ui.Gen1OffsetEndSpinBox->setEnabled(false);
		m_ui.Gen1OffsetEndSpinBox->setMinimum(0);
		m_ui.Gen1OffsetEndSpinBox->setMaximum(0);
		m_ui.Gen1OffsetEndSpinBox->setValue(0);
		m_ui.Gen1LoopRangeLabel->setEnabled(false);
		m_ui.Gen1LoopStartSpinBox->setEnabled(false);
		m_ui.Gen1LoopStartSpinBox->setMinimum(0);
		m_ui.Gen1LoopStartSpinBox->setMaximum(0);
		m_ui.Gen1LoopStartSpinBox->setValue(0);
		m_ui.Gen1LoopEndSpinBox->setEnabled(false);
		m_ui.Gen1LoopEndSpinBox->setMinimum(0);
		m_ui.Gen1LoopEndSpinBox->setMaximum(0);
		m_ui.Gen1LoopEndSpinBox->setValue(0);
		m_ui.Gen1LoopFadeCheckBox->setEnabled(false);
		m_ui.Gen1LoopFadeSpinBox->setEnabled(false);
		m_ui.Gen1LoopFadeSpinBox->setMinimum(0);
		m_ui.Gen1LoopFadeSpinBox->setMaximum(0);
		m_ui.Gen1LoopFadeSpinBox->setValue(0);
		m_ui.Gen1LoopZeroCheckBox->setEnabled(false);
		m_ui.Gen1Sample->setOffsetStart(0);
		m_ui.Gen1Sample->setOffsetEnd(0);
		m_ui.Gen1Sample->setOffset(false);
		m_ui.Gen1Sample->setLoopStart(0);
		m_ui.Gen1Sample->setLoopEnd(0);
		m_ui.Gen1Sample->setLoop(false);
	}
}


// (En|Dis)able all param/knobs.
void samplv1widget::activateParamKnobs ( bool bEnabled )
{
	activateParamKnobsGroupBox(m_ui.Gen1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Dcf1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Lfo1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Dca1GroupBox, bEnabled);
	activateParamKnobsGroupBox(m_ui.Out1GroupBox, bEnabled);

	m_ui.StatusBar->keybd()->setEnabled(bEnabled);

	m_ui.Gen1Sample->setEnabled(true);
}


void samplv1widget::activateParamKnobsGroupBox (
	QGroupBox *pGroupBox, bool bEnabled )
{
	if (pGroupBox->isCheckable()) {
		pGroupBox->setEnabled(bEnabled);
	} else {
		const QList<QWidget *>& children
			= pGroupBox->findChildren<QWidget *> ();
		QListIterator<QWidget *> iter(children);
		while (iter.hasNext())
			iter.next()->setEnabled(bEnabled);
	}
}


// Sample context menu.
void samplv1widget::contextMenuRequest ( const QPoint& pos )
{
	QMenu menu(this);
	QAction *pAction;

	samplv1_ui *pSamplUi = ui_instance();
	const char *pszSampleFile = nullptr;
	if (pSamplUi)
		pszSampleFile = pSamplUi->sampleFile();

	pAction = menu.addAction(
		QIcon(":/images/fileOpen.png"),
		tr("Open Sample..."), this, SLOT(openSample()));
	pAction->setEnabled(pSamplUi != nullptr);
	pAction = menu.addAction(
		QIcon(":/images/playSample.png"),
		tr("Play"), this, SLOT(playSample()));
	pAction->setEnabled(pSamplUi != nullptr);
	menu.addSeparator();
	pAction = menu.addAction(
		tr("Reset"), this, SLOT(clearSample()));
	pAction->setEnabled(pszSampleFile != nullptr);

	menu.exec(static_cast<QWidget *> (sender())->mapToGlobal(pos));
}


// Preset status updater.
void samplv1widget::updateLoadPreset ( const QString& sPreset )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		updateSample(pSamplUi->sample());

	resetParamKnobs();
	updateParamValues();

	m_ui.Preset->setPreset(sPreset);
	m_ui.StatusBar->showMessage(tr("Load preset: %1").arg(sPreset), 5000);
	updateDirtyPreset(false);
}


// Notification updater.
void samplv1widget::updateSchedNotify ( int stype, int sid )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

#ifdef CONFIG_DEBUG_0
	qDebug("samplv1widget::updateSchedNotify(%d, 0x%04x)", stype, sid);
#endif

	switch (samplv1_sched::Type(stype)) {
	case samplv1_sched::MidiIn:
		if (sid >= 0) {
			const int key = (sid & 0x7f);
			const int vel = (sid >> 7) & 0x7f;
			m_ui.StatusBar->midiInNote(key, vel);
		}
		else
		if (pSamplUi->midiInCount() > 0) {
			m_ui.StatusBar->midiInLed(true);
			QTimer::singleShot(200, this, SLOT(midiInLedTimeout()));
		}
		break;
	case samplv1_sched::Controller: {
		samplv1widget_control *pInstance
			= samplv1widget_control::getInstance();
		if (pInstance) {
			samplv1_controls *pControls = pSamplUi->controls();
			pInstance->setControlKey(pControls->current_key());
		}
		break;
	}
	case samplv1_sched::Controls: {
		const samplv1::ParamIndex index = samplv1::ParamIndex(sid);
		updateSchedParam(index, pSamplUi->paramValue(index));
		break;
	}
	case samplv1_sched::Programs: {
		samplv1_programs *pPrograms = pSamplUi->programs();
		samplv1_programs::Prog *pProg = pPrograms->current_prog();
		if (pProg) updateLoadPreset(pProg->name());
		break;
	}
	case samplv1_sched::Sample:
		updateSample(pSamplUi->sample());
		if (sid > 0) {
			updateParamValues();
			resetParamKnobs();
			updateDirtyPreset(false);
		}
		// Fall thru...
	default:
		break;
	}
}


// Direct note-on/off slot.
void samplv1widget::directNoteOn ( int iNote, int iVelocity )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::directNoteOn(%d, %d)", iNote, iVelocity);
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->directNoteOn(iNote, iVelocity); // note-on!
}


// Keyboard note range change.
void samplv1widget::noteRangeChanged (void)
{
	const int iNoteLow  = m_ui.StatusBar->keybd()->noteLow();
	const int iNoteHigh = m_ui.StatusBar->keybd()->noteHigh();

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::noteRangeChanged(%d, %d)", iNoteLow, iNoteHigh);
#endif

	updateParam(samplv1::KEY1_LOW,  float(iNoteLow));
	updateParam(samplv1::KEY1_HIGH, float(iNoteHigh));

	m_ui.StatusBar->showMessage(QString("KEY Low: %1 (%2) High: %3 (%4)")
		.arg(samplv1_ui::noteName(iNoteLow)).arg(iNoteLow)
		.arg(samplv1_ui::noteName(iNoteHigh)).arg(iNoteHigh), 5000);

	updateDirtyPreset(true);
}


// MIDI In LED timeout.
void samplv1widget::midiInLedTimeout (void)
{
	m_ui.StatusBar->midiInLed(false);
}


// Menu actions.
void samplv1widget::helpConfigure (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	samplv1widget_config(pSamplUi, this).exec();
}


void samplv1widget::helpAbout (void)
{
	// About...
	QStringList list;
#ifdef CONFIG_DEBUG
	list << tr("Debugging option enabled.");
#endif
#ifndef CONFIG_JACK
	list << tr("JACK stand-alone build disabled.");
#endif
#ifndef CONFIG_JACK_SESSION
	list << tr("JACK session support disabled.");
#endif
#ifndef CONFIG_JACK_MIDI
	list << tr("JACK MIDI support disabled.");
#endif
#ifndef CONFIG_ALSA_MIDI
	list << tr("ALSA MIDI support disabled.");
#endif
#ifndef CONFIG_LV2
	list << tr("LV2 plug-in build disabled.");
#endif

	QString sText = "<p>\n";
	sText += "<b>" SAMPLV1_TITLE "</b> - " + tr(SAMPLV1_SUBTITLE) + "<br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" CONFIG_BUILD_VERSION "</b><br />\n";
//	sText += "<small>" + tr("Build") + ": " CONFIG_BUILD_DATE "</small><br />\n";
	if (!list.isEmpty()) {
		sText += "<small><font color=\"red\">";
		sText += list.join("<br />\n");
		sText += "</font></small><br />\n";
	}
	sText += "<br />\n";
	sText += tr("Website") + ": <a href=\"" SAMPLV1_WEBSITE "\">" SAMPLV1_WEBSITE "</a><br />\n";
	sText += "<br />\n";
	sText += "<small>";
	sText += SAMPLV1_COPYRIGHT "<br />\n";
	sText += "<br />\n";
	sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	sText += tr("under the terms of the GNU General Public License version 2 or later.");
	sText += "</small>";
	sText += "</p>\n";

	QMessageBox::about(this, tr("About"), sText);
}


void samplv1widget::helpAboutQt (void)
{
	// About Qt...
	QMessageBox::aboutQt(this);
}



// Dirty flag (overridable virtual) methods.
void samplv1widget::updateDirtyPreset ( bool bDirtyPreset )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		pSamplUi->updatePreset(bDirtyPreset);

	m_ui.StatusBar->modified(bDirtyPreset);
	m_ui.Preset->setDirtyPreset(bDirtyPreset);
}


// Param knob context menu.
void samplv1widget::paramContextMenu ( const QPoint& pos )
{
	samplv1widget_param *pParam
		= qobject_cast<samplv1widget_param *> (sender());
	if (pParam == nullptr)
		return;

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == nullptr)
		return;

	samplv1_controls *pControls = pSamplUi->controls();
	if (pControls == nullptr)
		return;

	if (!pControls->enabled())
		return;

	QMenu menu(this);

	QAction *pAction = menu.addAction(
		QIcon(":/images/samplv1_control.png"),
		tr("MIDI &Controller..."));

	if (menu.exec(pParam->mapToGlobal(pos)) == pAction) {
		const samplv1::ParamIndex index = m_knobParams.value(pParam);
		const QString& sTitle = pParam->toolTip();
		samplv1widget_control::showInstance(pControls, index, sTitle, this);
	}
}


// Format changes (spinbox).
void samplv1widget::spinboxContextMenu ( const QPoint& pos )
{
	samplv1widget_spinbox *pSpinBox
		= qobject_cast<samplv1widget_spinbox *> (sender());
	if (pSpinBox == nullptr)
		return;

	samplv1widget_spinbox::Format format = pSpinBox->format();

	QMenu menu(this);
	QAction *pAction;

	pAction = menu.addAction(tr("&Frames"));
	pAction->setCheckable(true);
	pAction->setChecked(format == samplv1widget_spinbox::Frames);
	pAction->setData(int(samplv1widget_spinbox::Frames));

	pAction = menu.addAction(tr("&Time"));
	pAction->setCheckable(true);
	pAction->setChecked(format == samplv1widget_spinbox::Time);
	pAction->setData(int(samplv1widget_spinbox::Time));

	pAction = menu.exec(pSpinBox->mapToGlobal(pos));
	if (pAction == nullptr)
		return;

	format = samplv1widget_spinbox::Format(pAction->data().toInt());
	if (format != pSpinBox->format()) {
		samplv1_config *pConfig = samplv1_config::getInstance();
		if (pConfig) {
			pConfig->iFrameTimeFormat = int(format);
			m_ui.Gen1OffsetStartSpinBox->setFormat(format);
			m_ui.Gen1OffsetEndSpinBox->setFormat(format);
			m_ui.Gen1LoopStartSpinBox->setFormat(format);
			m_ui.Gen1LoopEndSpinBox->setFormat(format);
		}
	}
}


// end of samplv1widget.cpp
