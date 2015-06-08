// samplv1widget.cpp
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <QMessageBox>
#include <QDir>


//-------------------------------------------------------------------------
// samplv1widget - impl.
//

// Constructor.
samplv1widget::samplv1widget ( QWidget *pParent, Qt::WindowFlags wflags )
	: QWidget(pParent, wflags)
{
	Q_INIT_RESOURCE(samplv1);

#if QT_VERSION >= 0x050000
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
	m_sched_notifier = NULL;

	// Init swapable params A/B to default.
	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i)
		m_params_ab[i] = samplv1_param::paramDefaultValue(samplv1::ParamIndex(i));

	// Start clean.
	m_iUpdate = 0;

	// Replicate the stacked/pages
	for (int iTab = 0; iTab < m_ui.StackedWidget->count(); ++iTab)
		m_ui.TabBar->addTab(m_ui.StackedWidget->widget(iTab)->windowTitle());

	// Loop range font.
	const QFont& font = m_ui.Gen1LoopKnob->font();
	m_ui.Gen1LoopRangeFrame->setFont(font);

	const QFontMetrics fm(font);
	m_ui.Gen1LoopStartSpinBox->setMaximumHeight(fm.height() + 6);
	m_ui.Gen1LoopEndSpinBox->setMaximumHeight(fm.height() + 6);

	m_ui.Gen1LoopStartSpinBox->setAccelerated(true);
	m_ui.Gen1LoopEndSpinBox->setAccelerated(true);

	m_ui.Gen1LoopStartSpinBox->setMinimum(0);
	m_ui.Gen1LoopEndSpinBox->setMinimum(0);

	// Note names.
	QStringList notes;
	for (int note = 0; note < 128; ++note)
		notes << noteName(note);

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

	m_ui.Dcf1SlopeKnob->insertItems(0, slopes);

	// Dynamic states.
	QStringList states;
	states << tr("Off");
	states << tr("On");

	m_ui.Gen1ReverseKnob->insertItems(0, states);
	m_ui.Gen1LoopKnob->insertItems(0, states);

	m_ui.Dyn1CompressKnob->insertItems(0, states);
	m_ui.Dyn1LimiterKnob->insertItems(0, states);

	// Special values
	const QString& sOff = states.first();
	m_ui.Cho1WetKnob->setSpecialValueText(sOff);
	m_ui.Fla1WetKnob->setSpecialValueText(sOff);
	m_ui.Pha1WetKnob->setSpecialValueText(sOff);
	m_ui.Del1WetKnob->setSpecialValueText(sOff);
	m_ui.Rev1WetKnob->setSpecialValueText(sOff);

	const QString& sAuto = tr("Auto");
	m_ui.Gen1EnvTimeKnob->setSpecialValueText(sAuto);
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

	// Mono switches
	m_ui.Def1MonoKnob->insertItems(0, states);

	// Output (stereo-)width limits.
	m_ui.Out1WidthKnob->setMinimum(-1.0f);
	m_ui.Out1WidthKnob->setMaximum(+1.0f);

	// Output (stereo-)panning limits.
	m_ui.Out1PanningKnob->setMinimum(-1.0f);
	m_ui.Out1PanningKnob->setMaximum(+1.0f);

	// Effects (delay BPM)
	m_ui.Del1BpmKnob->setScale(1.0f);
	m_ui.Del1BpmKnob->setMinimum(3.6f);
	m_ui.Del1BpmKnob->setMaximum(360.0f);
	m_ui.Del1BpmKnob->setSingleStep(1.0f);

	// Reverb (stereo-)width limits.
	m_ui.Rev1WidthKnob->setMinimum(-1.0f);
	m_ui.Rev1WidthKnob->setMaximum(+1.0f);

	// GEN1
	setParamKnob(samplv1::GEN1_SAMPLE,  m_ui.Gen1SampleKnob);
	setParamKnob(samplv1::GEN1_REVERSE, m_ui.Gen1ReverseKnob);
	setParamKnob(samplv1::GEN1_LOOP,    m_ui.Gen1LoopKnob);
	setParamKnob(samplv1::GEN1_OCTAVE,  m_ui.Gen1OctaveKnob);
	setParamKnob(samplv1::GEN1_TUNING,  m_ui.Gen1TuningKnob);
	setParamKnob(samplv1::GEN1_GLIDE,   m_ui.Gen1GlideKnob);
	setParamKnob(samplv1::GEN1_ENVTIME, m_ui.Gen1EnvTimeKnob);

	// DCF1
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
	setParamKnob(samplv1::LFO1_SHAPE,   m_ui.Lfo1ShapeKnob);
	setParamKnob(samplv1::LFO1_WIDTH,   m_ui.Lfo1WidthKnob);
	setParamKnob(samplv1::LFO1_RATE,    m_ui.Lfo1RateKnob);
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
	setParamKnob(samplv1::OUT1_VOLUME,  m_ui.Out1VolumeKnob);


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

	QObject::connect(m_ui.Del1BpmKnob,
		SIGNAL(valueChanged(float)),
		SLOT(bpmSyncChanged()));

	// Reverb
	setParamKnob(samplv1::REV1_WET,   m_ui.Rev1WetKnob);
	setParamKnob(samplv1::REV1_ROOM,  m_ui.Rev1RoomKnob);
	setParamKnob(samplv1::REV1_DAMP,  m_ui.Rev1DampKnob);
	setParamKnob(samplv1::REV1_FEEDB, m_ui.Rev1FeedbKnob);
	setParamKnob(samplv1::REV1_WIDTH, m_ui.Rev1WidthKnob);

	// Dynamics
	setParamKnob(samplv1::DYN1_COMPRESS, m_ui.Dyn1CompressKnob);
	setParamKnob(samplv1::DYN1_LIMITER,  m_ui.Dyn1LimiterKnob);


	// Sample management...
	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(loadSampleFile(const QString&)),
		SLOT(loadSample(const QString&)));

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

	// Sample context menu...
	m_ui.Gen1Sample->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(contextMenuRequest(const QPoint&)));

	QObject::connect(m_ui.Gen1Sample,
		SIGNAL(loopRangeChanged()),
		SLOT(loopRangeChanged()));

	QObject::connect(m_ui.Gen1LoopStartSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(loopStartChanged()));
	QObject::connect(m_ui.Gen1LoopEndSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(loopEndChanged()));

	// Swap params A/B
	QObject::connect(m_ui.SwapParamsAButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));
	QObject::connect(m_ui.SwapParamsBButton,
		SIGNAL(toggled(bool)),
		SLOT(swapParams(bool)));

	// Direct stacked-page signal/slot
	QObject::connect(m_ui.TabBar, SIGNAL(currentChanged(int)),
		m_ui.StackedWidget, SLOT(setCurrentIndex(int)));

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

	// General knob/dial  behavior init...
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig) {
		samplv1widget_dial::setDialMode(
			samplv1widget_dial::DialMode(pConfig->iKnobDialMode));
	}

	// Epilog.
	// QWidget::adjustSize();

	m_ui.StatusBar->showMessage(tr("Ready"), 5000);
	m_ui.StatusBar->setModified(false);
	m_ui.Preset->setDirtyPreset(false);
}


// Destructor.
samplv1widget::~samplv1widget (void)
{
	if (m_sched_notifier)
		delete m_sched_notifier;
}


// Create/initialize the scheduler/work notifier.
void samplv1widget::initSchedNotifier (void)
{
	if (m_sched_notifier) {
		delete m_sched_notifier;
		m_sched_notifier = NULL;
	}

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

	m_sched_notifier = new samplv1widget_sched(pSamplUi->instance(), this);

	QObject::connect(m_sched_notifier,
		SIGNAL(notify(int, int)),
		SLOT(updateSchedNotify(int, int)));
}


// Param kbob (widget) map accesors.
void samplv1widget::setParamKnob ( samplv1::ParamIndex index, samplv1widget_knob *pKnob )
{
	pKnob->setDefaultValue(samplv1_param::paramDefaultValue(index));

	m_paramKnobs.insert(index, pKnob);
	m_knobParams.insert(pKnob, index);

	QObject::connect(pKnob,
		SIGNAL(valueChanged(float)),
		SLOT(paramChanged(float)));

	pKnob->setContextMenuPolicy(Qt::CustomContextMenu);

	QObject::connect(pKnob,
		SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(paramContextMenu(const QPoint&)));
}

samplv1widget_knob *samplv1widget::paramKnob ( samplv1::ParamIndex index ) const
{
	return m_paramKnobs.value(index, NULL);
}


// Param port accessors.
void samplv1widget::setParamValue (
	samplv1::ParamIndex index, float fValue, bool bDefault )
{
	++m_iUpdate;

	samplv1widget_knob *pKnob = paramKnob(index);
	if (pKnob)
		pKnob->setValue(fValue, bDefault);

	updateParamEx(index, fValue);

	--m_iUpdate;
}

float samplv1widget::paramValue ( samplv1::ParamIndex index ) const
{
	float fValue = 0.0f;

	samplv1widget_knob *pKnob = paramKnob(index);
	if (pKnob) {
		fValue = pKnob->value();
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

	samplv1widget_knob *pKnob = qobject_cast<samplv1widget_knob *> (sender());
	if (pKnob) {
		samplv1::ParamIndex index = m_knobParams.value(pKnob);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_ui.StatusBar->showMessage(QString("%1: %2")
			.arg(pKnob->toolTip())
			.arg(pKnob->valueText()), 5000);
		updateDirtyPreset(true);
	}
}


// Update local tied widgets.
void samplv1widget::updateParamEx ( samplv1::ParamIndex index, float fValue )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

	++m_iUpdate;

	switch (index) {
#if 1//--updateSchedNotify(samplv1_sched::Sample, 0);
	case samplv1::GEN1_REVERSE: {
		const bool bReverse = bool(fValue > 0.0f);
		pSamplUi->setReverse(bReverse);
		updateSample(pSamplUi->sample());
		break;
	}
#endif
	case samplv1::GEN1_LOOP: {
		const bool bLoop = bool(fValue > 0.0f);
		pSamplUi->setLoop(bLoop);
		m_ui.Gen1Sample->setLoop(bLoop);
		m_ui.Gen1Sample->setLoopStart(pSamplUi->loopStart());
		m_ui.Gen1Sample->setLoopEnd(pSamplUi->loopEnd());
		m_ui.Gen1LoopRangeFrame->setEnabled(bLoop);
		updateSampleLoop(pSamplUi->sample());
		break;
	}
	case samplv1::DEL1_BPMSYNC:
		if (fValue > 0.0f)
			m_ui.Del1BpmKnob->setValue(0.0f);
		// Fall thru...
	default:
		break;
	}

	--m_iUpdate;
}


// Update scheduled controllers param/knob widgets.
void samplv1widget::updateSchedParam ( samplv1::ParamIndex index, float fValue )
{
	++m_iUpdate;

	samplv1widget_knob *pKnob = paramKnob(index);
	if (pKnob) {
		pKnob->setValue(fValue, false);
		updateParam(index, fValue);
		updateParamEx(index, fValue);
		m_ui.StatusBar->showMessage(QString("%1: %2")
			.arg(pKnob->toolTip())
			.arg(pKnob->valueText()), 5000);
		updateDirtyPreset(true);
	}

	--m_iUpdate;
}


// Reset all param knobs to default values.
void samplv1widget::resetParams (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

	pSamplUi->reset();

	resetSwapParams();

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1::ParamIndex index = samplv1::ParamIndex(i);
		float fValue = samplv1_param::paramDefaultValue(index);
		samplv1widget_knob *pKnob = paramKnob(index);
		if (pKnob)
			fValue = pKnob->defaultValue();
		setParamValue(index, fValue);
		updateParam(index, fValue);
		m_params_ab[index] = fValue;
	}

	m_ui.StatusBar->showMessage(tr("Reset preset"), 5000);
	updateDirtyPreset(false);
}


// Swap params A/B.
void samplv1widget::swapParams ( bool bOn )
{
	if (m_iUpdate > 0 || !bOn)
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::swapParams(%d)", int(bOn));
#endif
//	resetParamKnobs();

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1::ParamIndex index = samplv1::ParamIndex(i);
		samplv1widget_knob *pKnob = paramKnob(index);
		if (pKnob) {
			const float fOldValue = pKnob->value();
			const float fNewValue = m_params_ab[index];
			setParamValue(index, fNewValue);
			updateParam(index, fNewValue);
			m_params_ab[index] = fOldValue;
		}
	}

	const bool bSwapA = m_ui.SwapParamsAButton->isChecked();
	m_ui.StatusBar->showMessage(tr("Swap %1").arg(bSwapA ? 'A' : 'B'), 5000);
	updateDirtyPreset(true);
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

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1::ParamIndex index = samplv1::ParamIndex(i);
		const float fValue = (pSamplUi
			? pSamplUi->paramValue(index)
			: samplv1_param::paramDefaultValue(index));
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
	//	updateParamEx(index, fValue);
		m_params_ab[index] = fValue;
	}
}


// Reset all param default values.
void samplv1widget::resetParamValues (void)
{
	resetSwapParams();

	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1::ParamIndex index = samplv1::ParamIndex(i);
		const float fValue = samplv1_param::paramDefaultValue(index);
		setParamValue(index, fValue, true);
		updateParam(index, fValue);
		m_params_ab[index] = fValue;
	}
}


// Reset all knob default values.
void samplv1widget::resetParamKnobs (void)
{
	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		samplv1widget_knob *pKnob = paramKnob(samplv1::ParamIndex(i));
		if (pKnob)
			pKnob->resetDefaultValue();
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

	m_ui.Gen1Sample->openSample();
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
		samplv1_param::loadPreset(pSamplUi->instance(), sFilename);

	updateLoadPreset(QFileInfo(sFilename).completeBaseName());
}


void samplv1widget::savePreset ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::savePreset(\"%s\")", sFilename.toUtf8().constData());
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		samplv1_param::savePreset(pSamplUi->instance(), sFilename);

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


// Sample file loader slot.
void samplv1widget::loadSample ( const QString& sFilename )
{
	loadSampleFile(QFileInfo(sFilename).canonicalFilePath());

	m_ui.StatusBar->showMessage(tr("Load sample: %1").arg(sFilename), 5000);
	updateDirtyPreset(true);
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
		pSamplUi->setSampleFile(0);

	updateSample(0);
}


// Sample file loader.
void samplv1widget::loadSampleFile ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::loadSampleFile(\"%s\")", sFilename.toUtf8().constData());
#endif

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		pSamplUi->setSampleFile(sFilename.toUtf8().constData());
		updateSample(pSamplUi->sample());
	}
}


// Sample updater (crude experimental stuff II).
void samplv1widget::updateSample ( samplv1_sample *pSample, bool bDirty )
{
	m_ui.Gen1Sample->setSample(pSample);

	++m_iUpdate;
	if (pSample) {
		const bool bLoop = pSample->isLoop();
		m_ui.Gen1Sample->setLoop(bLoop);
		const uint32_t iLoopStart = pSample->loopStart();
		const uint32_t iLoopEnd = pSample->loopEnd();
		m_ui.Gen1Sample->setLoopStart(iLoopStart);
		m_ui.Gen1Sample->setLoopEnd(iLoopEnd);
		updateSampleLoop(pSample);
	} else {
		m_ui.Gen1Sample->setLoop(false);
		m_ui.Gen1Sample->setLoopStart(0);
		m_ui.Gen1Sample->setLoopEnd(0);
		updateSampleLoop(NULL);
	}
	--m_iUpdate;

	if (pSample && bDirty)
		updateDirtyPreset(true);
}


// MIDI note/octave name helper (static).
QString samplv1widget::noteName ( int note )
{
	static const char *notes[] =
		{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	return QString("%1 %2").arg(notes[note % 12]).arg((note / 12) - 1);
}


// Dirty close prompt,
bool samplv1widget::queryClose (void)
{
	return m_ui.Preset->queryPreset();
}


// Loop range change.
void samplv1widget::loopRangeChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const uint32_t iLoopStart = m_ui.Gen1Sample->loopStart();
		const uint32_t iLoopEnd = m_ui.Gen1Sample->loopEnd();
		pSamplUi->setLoopRange(iLoopStart, iLoopEnd);
		updateSampleLoop(pSamplUi->sample(), true);
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
		updateSampleLoop(pSamplUi->sample(), true);
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
		updateSampleLoop(pSamplUi->sample(), true);
	}
	--m_iUpdate;
}


void samplv1widget::updateSampleLoop ( samplv1_sample *pSample, bool bDirty )
{
	if (pSample) {
		const uint32_t iLoopStart = pSample->loopStart();
		const uint32_t iLoopEnd = pSample->loopEnd();
		const uint32_t nframes = pSample->length();
		m_ui.Gen1LoopStartSpinBox->setMinimum(0);
		m_ui.Gen1LoopStartSpinBox->setMaximum(
			iLoopEnd > 0 ? iLoopEnd - 1 : 0);
		m_ui.Gen1LoopEndSpinBox->setMinimum(
			iLoopStart < nframes ? iLoopStart + 1 : nframes);
		m_ui.Gen1LoopEndSpinBox->setMaximum(nframes);
		m_ui.Gen1LoopStartSpinBox->setValue(iLoopStart);
		m_ui.Gen1LoopEndSpinBox->setValue(iLoopEnd);
		if (bDirty) {
			m_ui.StatusBar->showMessage(tr("Loop start: %1, end: %2")
				.arg(iLoopStart).arg(iLoopEnd), 5000);
			updateDirtyPreset(true);
		}
	} else {
		m_ui.Gen1LoopStartSpinBox->setMinimum(0);
		m_ui.Gen1LoopStartSpinBox->setMaximum(0);
		m_ui.Gen1LoopStartSpinBox->setValue(0);
		m_ui.Gen1LoopEndSpinBox->setMinimum(0);
		m_ui.Gen1LoopEndSpinBox->setMaximum(0);
		m_ui.Gen1LoopEndSpinBox->setValue(0);
	}
}


// Delay BPM change.
void samplv1widget::bpmSyncChanged (void)
{
	if (m_iUpdate > 0)
		return;

	++m_iUpdate;
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi) {
		const bool bBpmSync0 = (pSamplUi->paramValue(samplv1::DEL1_BPMSYNC) > 0.0f);
		const bool bBpmSync1 = m_ui.Del1BpmKnob->isSpecialValue();
		if ((bBpmSync1 && !bBpmSync0) || (!bBpmSync1 && bBpmSync0))
			pSamplUi->setParamValue(samplv1::DEL1_BPMSYNC, (bBpmSync1 ? 1.0f : 0.0f));
	}
	--m_iUpdate;
}


// Sample context menu.
void samplv1widget::contextMenuRequest ( const QPoint& pos )
{
	QMenu menu(this);
	QAction *pAction;

	samplv1_ui *pSamplUi = ui_instance();
	const char *pszSampleFile = NULL;
	if (pSamplUi)
		pszSampleFile = pSamplUi->sampleFile();

	pAction = menu.addAction(
		QIcon(":/images/fileOpen.png"),
		tr("Open Sample..."), this, SLOT(openSample()));
	pAction->setEnabled(pSamplUi != NULL);
	menu.addSeparator();
	pAction = menu.addAction(
		tr("Reset"), this, SLOT(clearSample()));
	pAction->setEnabled(pszSampleFile != NULL);

	menu.exec(static_cast<QWidget *> (sender())->mapToGlobal(pos));
}


// Preset status updater.
void samplv1widget::updateLoadPreset ( const QString& sPreset )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi)
		updateSample(pSamplUi->sample());

	updateParamValues();

	m_ui.Preset->setPreset(sPreset);
	m_ui.StatusBar->showMessage(tr("Load preset: %1").arg(sPreset), 5000);
	updateDirtyPreset(false);
}


// Notification updater.
void samplv1widget::updateSchedNotify ( int stype, int sid )
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget::updateSchedNotify(%d, %d)", stype, sid);
#endif

	switch (samplv1_sched::Type(stype)) {
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
		// Fall thru...
	default:
		break;
	}
}


// Menu actions.
void samplv1widget::helpConfigure (void)
{
	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

	samplv1widget_config form(this);

	// Set controllers&&programs database...
	form.setControls(pSamplUi->controls());
	form.setPrograms(pSamplUi->programs());

	form.exec();
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
	sText += tr("Version") + ": <b>" SAMPLV1_VERSION "</b><br />\n";
	sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
	QStringListIterator iter(list);
	while (iter.hasNext()) {
		sText += "<small><font color=\"red\">";
		sText += iter.next();
		sText += "</font></small><br />";
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

	QMessageBox::about(this, tr("About") + " " SAMPLV1_TITLE, sText);
}


void samplv1widget::helpAboutQt (void)
{
	// About Qt...
	QMessageBox::aboutQt(this);
}


// Dirty flag (overridable virtual) methods.
void samplv1widget::updateDirtyPreset ( bool bDirtyPreset )
{
	m_ui.StatusBar->setModified(bDirtyPreset);
	m_ui.Preset->setDirtyPreset(bDirtyPreset);
}


// Param knob context menu.
void samplv1widget::paramContextMenu ( const QPoint& pos )
{
	samplv1widget_knob *pKnob
		= qobject_cast<samplv1widget_knob *> (sender());
	if (pKnob == NULL)
		return;

	samplv1_ui *pSamplUi = ui_instance();
	if (pSamplUi == NULL)
		return;

	samplv1_controls *pControls = pSamplUi->controls();
	if (pControls == NULL)
		return;

	QMenu menu(this);

	QAction *pAction = menu.addAction(
		QIcon(":/images/samplv1_control.png"),
		tr("MIDI &Controller..."));

	if (menu.exec(pKnob->mapToGlobal(pos)) == pAction) {
		const samplv1::ParamIndex index = m_knobParams.value(pKnob);
		const QString& sTitle = pKnob->toolTip();
		samplv1widget_control::showInstance(pControls, index, sTitle, this);
	}
}


// end of samplv1widget.cpp
