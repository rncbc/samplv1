// samplv1_ui.h
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

#ifndef __samplv1_ui_h
#define __samplv1_ui_h

#include "samplv1.h"

#include <QString>


//-------------------------------------------------------------------------
// samplv1_ui - decl.
//

class samplv1_ui
{
public:

	samplv1_ui(samplv1 *pSampl, bool bPlugin);

	samplv1 *instance() const;

	bool isPlugin() const;

	void setSampleFile(const char *pszSampleFile, uint16_t iOctaves);
	const char *sampleFile() const;
	uint16_t octaves() const;

	samplv1_sample *sample() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setOffset(bool bOffset);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	void setLoop(bool bLoop);
	bool isLoop() const;

	void setLoopRange(uint32_t iLoopStart, uint32_t iLoopEnd);
	uint32_t loopStart() const;
	uint32_t loopEnd() const;

	void setLoopFade(uint32_t iLoopFade);
	uint32_t loopFade() const;

	void setLoopZero(bool bLoopZero);
	bool isLoopZero() const;

	bool loadPreset(const QString& sFilename);
	bool savePreset(const QString& sFilename);

	void setParamValue(samplv1::ParamIndex index, float fValue);
	float paramValue(samplv1::ParamIndex index) const;

	samplv1_controls *controls() const;
	samplv1_programs *programs() const;

	void reset();

	void updatePreset(bool bDirty);

	void midiInEnabled(bool bEnabled);
	uint32_t midiInCount();

	void directNoteOn(int note, int vel);

	void setTuningEnabled(bool enabled);
	bool isTuningEnabled() const;

	void setTuningRefPitch(float refPitch);
	float tuningRefPitch() const;

	void setTuningRefNote(int refNote);
	int tuningRefNote() const;

	void setTuningScaleFile(const char *pszScaleFile);
	const char *tuningScaleFile() const;

	void setTuningKeyMapFile(const char *pszKeyMapFile);
	const char *tuningKeyMapFile() const;

	void resetTuning();

	// MIDI note/octave name helper.
	static QString noteName(int note);

private:

	samplv1 *m_pSampl;

	bool m_bPlugin;
};


#endif// __samplv1_ui_h

// end of samplv1_ui.h
