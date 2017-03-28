// samplv1_ui.h
//
/****************************************************************************
   Copyright (C) 2012-2017, rncbc aka Rui Nuno Capela. All rights reserved.

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


//-------------------------------------------------------------------------
// samplv1_ui - decl.
//

class samplv1_ui
{
public:

	samplv1_ui(samplv1 *pSampl);

	void setSampleFile(const char *pszSampleFile);
	const char *sampleFile() const;

	samplv1_sample *sample() const;

	void setReverse(bool bReverse);
	bool isReverse() const;

	void setLoop(bool bLoop);
	bool isLoop() const;

	void setLoopRange(uint32_t iLoopStart, uint32_t iLoopEnd);
	uint32_t loopStart() const;
	uint32_t loopEnd() const;

	samplv1 *instance() const;

	void setParamValue(samplv1::ParamIndex index, float fValue);
	float paramValue(samplv1::ParamIndex index) const;

	samplv1_controls *controls() const;
	samplv1_programs *programs() const;

	void reset();

	void updatePreset(bool bDirty);

	void midiInEnabled(bool bEnabled);
	uint32_t midiInCount();

private:

	samplv1 *m_pSampl;
};


#endif// __samplv1_ui_h

// end of samplv1_ui.h
