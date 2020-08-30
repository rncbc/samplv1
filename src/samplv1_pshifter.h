// samplv1_pshifter.h
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

#ifndef __samplv1_pshifter_h
#define __samplv1_pshifter_h

#include "config.h"

#include <stdint.h>

#ifdef CONFIG_LIBRUBBERBAND
// nothing to include.
#else
#ifdef CONFIG_FFTW3
#include <fftw3.h>
#endif
#endif	// CONFIG_LIBRUBBERBAND


//---------------------------------------------------------------------------
// samplv1_pshifter - Pitch-shift processor.
//

class samplv1_pshifter
{
public:

	// Constructor.
	samplv1_pshifter(
		uint16_t nchannels = 2,
		float srate = 44100.0f,
		uint32_t nsize = 4096,
		uint16_t nover = 8);
	
	// Destructor.
	~samplv1_pshifter();

	// Processor.
	void process(float **pframes, uint32_t nframes, float pshift);

protected:

	// Channel processor.
	void process_k(float *pframes, uint32_t nframes, float pshift);

private:

	// member variables
	uint16_t m_nchannels;
	float    m_srate;
	uint32_t m_nsize;
	uint16_t m_nover;

#ifdef CONFIG_LIBRUBBERBAND
	// nothing to decl.
#else
	float *m_fwind;
	float *m_ififo;
	float *m_ofifo;
#ifdef CONFIG_FFTW3
	float *m_iwork;
	float *m_owork;
#else
	float *m_fwork;
#endif
	float *m_plast;
	float *m_phase;
	float *m_accum;
	float *m_afreq;
	float *m_amagn;
	float *m_sfreq;
	float *m_smagn;
#ifdef CONFIG_FFTW3
	fftwf_plan m_aplan;
	fftwf_plan m_splan;
#endif

#endif	// !CONFIG_LIBRUBBERBAND
};


#endif  // __samplv1_pshifter_h


// end of samplv1_pshifter.h
