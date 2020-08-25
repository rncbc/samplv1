// samplv1widget.h
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

#ifndef __samplv1widget_h
#define __samplv1widget_h

#include "samplv1_config.h"
#include "samplv1_sched.h"

#include "samplv1_ui.h"

#include <QWidget>


// forward decls.
namespace Ui { class samplv1widget; }

class samplv1widget_param;
class samplv1widget_sched;

class QGroupBox;


//-------------------------------------------------------------------------
// samplv1widget - decl.
//

class samplv1widget : public QWidget
{
	Q_OBJECT

public:

	// Constructor
	samplv1widget(QWidget *pParent = nullptr);

	// Destructor.
	virtual ~samplv1widget();

	// Open/close the scheduler/work notifier.
	void openSchedNotifier();
	void closeSchedNotifier();

	// Param port accessors.
	void setParamValue(samplv1::ParamIndex index, float fValue, bool bIter = false);
	float paramValue(samplv1::ParamIndex index) const;

	// Param kbob (widget) mapper.
	void setParamKnob(samplv1::ParamIndex index, samplv1widget_param *pKnob);
	samplv1widget_param *paramKnob(samplv1::ParamIndex index) const;

	// Preset init.
	void initPreset();
	// Preset clear.
	void clearPreset();

	// Dirty close prompt,
	bool queryClose();

public slots:

	// Preset file I/O.
	void loadPreset(const QString& sFilename);
	void savePreset(const QString& sFilename);

	// Direct note-on/off slot.
	void directNoteOn(int iNote, int iVelocity);

protected slots:

	// Preset renewal.
	void newPreset();

	// Param knob (widget) slots.
	void paramChanged(float fValue);

	// Sample clear slot.
	void clearSample();

	// Sample openner.
	void openSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

	// Sample playback (direct note-on/off).
	void playSample(void);

	// Common context menu.
	void contextMenuRequest(const QPoint& pos);

	// Reset param knobs to default value.
	void resetParams();

	// Randomize params (partial).
	void randomParams();

	// Swap params A/B.
	void swapParams(bool bOn);

	// Panic: all-notes/sound-off (reset).
	void panic();
	
	// Octaves change slot.
	void octavesChanged(int iOctaves);

	// Offset point changed.
	void offsetStartChanged();
	void offsetEndChanged();

	// Loop point changes.
	void loopStartChanged();
	void loopEndChanged();
	void loopFadeChanged();
	void loopZeroChanged();

	// Offset/loop points changed (from UI).
	void offsetRangeChanged();
	void loopRangeChanged();

	// Notification updater.
	void updateSchedNotify(int stype, int sid);

	// MIDI In LED timeout.
	void midiInLedTimeout();

	// Keyboard note range change.
	void noteRangeChanged();

	// Param knob context menu.
	void paramContextMenu(const QPoint& pos);

	// Format changes (spinbox).
	void spinboxContextMenu(const QPoint& pos);

	// Menu actions.
	void helpConfigure();

	void helpAbout();
	void helpAboutQt();

protected:

	// Synth engine accessor.
	virtual samplv1_ui *ui_instance() const = 0;

	// Reset swap params A/B group.
	void resetSwapParams();

	// Initialize all param/knob values.
	void updateParamValues();

	// Reset all param/knob default values.
	void resetParamValues();
	void resetParamKnobs();

	// (En|Dis)able all param/knobs.
	void activateParamKnobs(bool bEnabled);
	void activateParamKnobsGroupBox(QGroupBox *pGroupBox, bool bEnable);

	// Sample file clearance.
	void clearSampleFile();

	// Sample loader.
	void loadSampleFile(const QString& sFilename, int iOctaves);

	// Sample updater.
	void updateSample(samplv1_sample *pSample, bool bDirty = false);

	// Update offset/loop range change status.
	void updateOffsetLoop(samplv1_sample *pSample, bool bDirty = false);

	// Param port methods.
	virtual void updateParam(samplv1::ParamIndex index, float fValue) const = 0;

	// Update local tied widgets.
	void updateParamEx(samplv1::ParamIndex index, float fValue, bool bIter = false);

	// Update scheduled controllers param/knob widgets.
	void updateSchedParam(samplv1::ParamIndex index, float fValue);

	// Preset status updater.
	void updateLoadPreset(const QString& sPreset);

	// Dirty flag (overridable virtual) methods.
	virtual void updateDirtyPreset(bool bDirtyPreset);

	// Show/hide dget handlers.
	void showEvent(QShowEvent *pShowEvent);
	void hideEvent(QHideEvent *pHideEvent);

private:

	// Instance variables.
	Ui::samplv1widget *p_ui;
	Ui::samplv1widget& m_ui;

	samplv1widget_sched *m_sched_notifier;

	QHash<samplv1::ParamIndex, samplv1widget_param *> m_paramKnobs;
	QHash<samplv1widget_param *, samplv1::ParamIndex> m_knobParams;

	float m_params_ab[samplv1::NUM_PARAMS];

	uint32_t m_iLoopFade;

	int m_iUpdate;
};


//-------------------------------------------------------------------------
// samplv1widget_sched - worker/schedule proxy decl.
//

class samplv1widget_sched : public QObject
{
	Q_OBJECT

public:

	// ctor.
	samplv1widget_sched(samplv1 *pSampl, QObject *pParent = nullptr)
		: QObject(pParent), m_notifier(pSampl, this) {}

signals:

	// Notification signal.
	void notify(int stype, int sid);

protected:

	// Notififier visitor.
	class Notifier : public samplv1_sched::Notifier
	{
	public:

		Notifier(samplv1 *pSampl, samplv1widget_sched *pSched)
			: samplv1_sched::Notifier(pSampl), m_pSched(pSched) {}

		void notify(samplv1_sched::Type stype, int sid) const
			{ m_pSched->emit_notify(stype, sid); }

	private:

		samplv1widget_sched *m_pSched;
	};

	// Notification method.
	void emit_notify(samplv1_sched::Type stype, int sid)
		{ emit notify(int(stype), sid); }

private:

	// Instance variables.
	Notifier m_notifier;
};


#endif	// __samplv1widget_h

// end of samplv1widget.h
