// samplv1widget.h
//
/****************************************************************************
   Copyright (C) 2012, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "ui_samplv1widget.h"

#include "samplv1widget_config.h"

#include "samplv1.h"

#include <QHash>


//-------------------------------------------------------------------------
// samplv1widget - decl.
//

class samplv1widget : public QWidget
{
	Q_OBJECT

public:

	// Constructor
	samplv1widget(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);

	// Param port accessors.
	void setParamValue(samplv1::ParamIndex index, float fValue);
	float paramValue(samplv1::ParamIndex index) const;

	// Param kbob (widget) mapper.
	void setParamKnob(samplv1::ParamIndex index, samplv1widget_knob *pKnob);
	samplv1widget_knob *paramKnob(samplv1::ParamIndex index) const;

	// MIDI note/octave name helper.
	static QString noteName(int note);

	// Dirty close prompt,
	bool queryClose();

public slots:

	// Preset file I/O.
	void loadPreset(const QString& sFilename);
	void savePreset(const QString& sFilename);

protected slots:

	// Preset clear.
	void newPreset();

	// Param knob (widget) slots.
	void paramChanged(float fValue);

	// Sample clear slot.
	void clearSample();

	// Sample openner.
	void openSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

	// Common context menu.
	void contextMenuRequest(const QPoint& pos);

	// Reset param knobs to default value.
	void resetParams();

	// Swap params A/B.
	void swapParams(bool bOn);

	// Menu actions.
	void helpAbout();
	void helpAboutQt();

protected:

	// Synth engine accessor.
	virtual samplv1 *instance() const = 0;

	// Preset init.
	void initPreset();

	// Reset all param/knob default values.
	void resetParamValues();
	void resetParamKnobs();

	// Param port methods.
	virtual void updateParam(samplv1::ParamIndex index, float fValue) const = 0;

	// Sample file clearance.
	void clearSampleFile();

	// Sample loader slot.
	void loadSampleFile(const QString& sFilename);

	// Sample filename retriever.
	QString sampleFile() const;

	// Sample updater.
	void updateSample(samplv1_sample *pSample, bool bDirty = false);

private:

	// Instance variables.
	Ui::samplv1widget m_ui;

	samplv1widget_config m_config;

	QHash<samplv1::ParamIndex, samplv1widget_knob *> m_paramKnobs;
	QHash<samplv1widget_knob *, samplv1::ParamIndex> m_knobParams;

	float m_params_ab[samplv1::NUM_PARAMS];

	int m_iUpdate;
};


#endif	// __samplv1widget_h

// end of samplv1widget.h
