// samplv1_ui.cpp
//
/****************************************************************************
   Copyright (C) 2012-2016, rncbc aka Rui Nuno Capela. All rights reserved.

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


//-------------------------------------------------------------------------
// samplv1_ui - decl.
//

samplv1_ui::samplv1_ui ( samplv1 *pSampl ) : m_pSampl(pSampl)
{
}


samplv1 *samplv1_ui::instance (void) const
{
	return m_pSampl;
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


// end of samplv1_ui.cpp
