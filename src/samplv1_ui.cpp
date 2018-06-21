// samplv1_ui.cpp
//
/****************************************************************************
   Copyright (C) 2012-2018, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_ui.h"

#include "samplv1_param.h"


//-------------------------------------------------------------------------
// samplv1_ui - decl.
//

samplv1_ui::samplv1_ui ( samplv1 *pSampl, bool bPlugin )
	: m_pSampl(pSampl), m_bPlugin(bPlugin)
{
}


samplv1 *samplv1_ui::instance (void) const
{
	return m_pSampl;
}


bool samplv1_ui::isPlugin (void) const
{
	return m_bPlugin;
}


void samplv1_ui::setSampleFile ( const char *pszSampleFile )
{
	m_pSampl->setSampleFile(pszSampleFile);
}

const char *samplv1_ui::sampleFile (void) const
{
	return m_pSampl->sampleFile();
}


samplv1_sample *samplv1_ui::sample (void) const
{
	return m_pSampl->sample();
}

void samplv1_ui::setReverse ( bool bReverse )
{
	m_pSampl->setReverse(bReverse);
}

bool samplv1_ui::isReverse (void) const
{
	return m_pSampl->isReverse();
}


void samplv1_ui::setLoop ( bool bLoop )
{
	m_pSampl->setLoop(bLoop);
}

bool samplv1_ui::isLoop (void) const
{
	return m_pSampl->isLoop();
}


void samplv1_ui::setLoopRange ( uint32_t iLoopStart, uint32_t iLoopEnd )
{
	m_pSampl->setLoopRange(iLoopStart, iLoopEnd);
}

uint32_t samplv1_ui::loopStart (void) const
{
	return m_pSampl->loopStart();
}

uint32_t samplv1_ui::loopEnd (void) const
{
	return m_pSampl->loopEnd();
}


void samplv1_ui::setLoopFade ( uint32_t iLoopFade )
{
	m_pSampl->setLoopFade(iLoopFade);
}

uint32_t samplv1_ui::loopFade (void) const
{
	return m_pSampl->loopFade();
}


void samplv1_ui::setLoopZero ( bool bLoopZero )
{
	m_pSampl->setLoopZero(bLoopZero);
}

bool samplv1_ui::isLoopZero (void) const
{
	return m_pSampl->isLoopZero();
}


bool samplv1_ui::loadPreset ( const QString& sFilename )
{
	return samplv1_param::loadPreset(m_pSampl, sFilename);
}

bool samplv1_ui::savePreset ( const QString& sFilename )
{
	return samplv1_param::savePreset(m_pSampl, sFilename);
}


void samplv1_ui::setParamValue ( samplv1::ParamIndex index, float fValue )
{
	m_pSampl->setParamValue(index, fValue);
}

float samplv1_ui::paramValue ( samplv1::ParamIndex index ) const
{
	return m_pSampl->paramValue(index);
}


samplv1_controls *samplv1_ui::controls (void) const
{
	return m_pSampl->controls();
}


samplv1_programs *samplv1_ui::programs (void) const
{
	return m_pSampl->programs();
}


void samplv1_ui::reset (void)
{
	return m_pSampl->reset();
}


void samplv1_ui::updatePreset ( bool bDirty )
{
	m_pSampl->updatePreset(bDirty);
}


void samplv1_ui::midiInEnabled ( bool bEnabled )
{
	m_pSampl->midiInEnabled(bEnabled);
}


uint32_t samplv1_ui::midiInCount (void)
{
	return m_pSampl->midiInCount();
}


void samplv1_ui::directNoteOn ( int note, int vel )
{
	m_pSampl->directNoteOn(note, vel);
}


void samplv1_ui::updateTuning (void)
{
	m_pSampl->updateTuning();
}


// MIDI note/octave name helper (static).
QString samplv1_ui::noteName ( int note )
{
	static const char *notes[] =
		{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	return QString("%1 %2").arg(notes[note % 12]).arg((note / 12) - 1);
}


// end of samplv1_ui.cpp
