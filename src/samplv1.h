// samplv1.h
//
/****************************************************************************
   Copyright (C) 2012-2022, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __samplv1_h
#define __samplv1_h

#include "config.h"

#include <cstdint>


// forward declarations
class samplv1_impl;
class samplv1_port;
class samplv1_sample;
class samplv1_controls;
class samplv1_programs;


//-------------------------------------------------------------------------
// samplv1 - decl.
//

class samplv1
{
public:

	samplv1(uint16_t nchannels = 2, float srate = 44100.0f, uint32_t nsize = 1024);

	virtual ~samplv1();

	void setChannels(uint16_t nchannels);
	uint16_t channels() const;

	void setSampleRate(float srate);
	float sampleRate() const;

	void setSampleFile(const char *pszSampleFile, uint16_t iOctaves, bool bSync = false);
	const char *sampleFile() const;
	uint16_t octaves() const;

	samplv1_sample *sample() const;

	void setReverse(bool bReverse, bool bSync = false);
	bool isReverse() const;

	void setOffset(bool bOffset, bool bSync = false);
	bool isOffset() const;

	void setOffsetRange(uint32_t iOffsetStart, uint32_t iOffsetEnd, bool bSync = false);
	uint32_t offsetStart() const;
	uint32_t offsetEnd() const;

	void setLoop(bool bLoop, bool bSync = false);
	bool isLoop() const;

	void setLoopRange(uint32_t iLoopStart, uint32_t iLoopEnd, bool bSync = false);
	uint32_t loopStart() const;
	uint32_t loopEnd() const;

	void setLoopFade(uint32_t iLoopFade, bool bSync = false);
	uint32_t loopFade() const;

	void setLoopZero(bool bLoopZero, bool bSync = false);
	bool isLoopZero() const;

	void setBufferSize(uint32_t nsize);
	uint32_t bufferSize() const;

	void setTempo(float bpm);
	float tempo() const;

	enum ParamIndex	 {

		GEN1_SAMPLE = 0,
		GEN1_REVERSE,
		GEN1_OFFSET,
		GEN1_OFFSET_1,
		GEN1_OFFSET_2,
		GEN1_LOOP,
		GEN1_LOOP_1,
		GEN1_LOOP_2,
		GEN1_OCTAVE,
		GEN1_TUNING,
		GEN1_GLIDE,
		GEN1_ENVTIME,
		DCF1_ENABLED,
		DCF1_CUTOFF,
		DCF1_RESO,
		DCF1_TYPE,
		DCF1_SLOPE,
		DCF1_ENVELOPE,
		DCF1_ATTACK,
		DCF1_DECAY,
		DCF1_SUSTAIN,
		DCF1_RELEASE,
		LFO1_ENABLED,
		LFO1_SHAPE,
		LFO1_WIDTH,
		LFO1_BPM,
		LFO1_RATE,
		LFO1_SYNC,
		LFO1_SWEEP,
		LFO1_PITCH,
		LFO1_CUTOFF,
		LFO1_RESO,
		LFO1_PANNING,
		LFO1_VOLUME,
		LFO1_ATTACK,
		LFO1_DECAY,
		LFO1_SUSTAIN,
		LFO1_RELEASE,
		DCA1_ENABLED,
		DCA1_VOLUME,
		DCA1_ATTACK,
		DCA1_DECAY,
		DCA1_SUSTAIN,
		DCA1_RELEASE,
		OUT1_WIDTH,
		OUT1_PANNING,
		OUT1_FXSEND,
		OUT1_VOLUME,

		DEF1_PITCHBEND,
		DEF1_MODWHEEL,
		DEF1_PRESSURE,
		DEF1_VELOCITY,
		DEF1_CHANNEL,
		DEF1_MONO,

		CHO1_WET,
		CHO1_DELAY,
		CHO1_FEEDB,
		CHO1_RATE,
		CHO1_MOD,
		FLA1_WET,
		FLA1_DELAY,
		FLA1_FEEDB,
		FLA1_DAFT,
		PHA1_WET,
		PHA1_RATE,
		PHA1_FEEDB,
		PHA1_DEPTH,
		PHA1_DAFT,
		DEL1_WET,
		DEL1_DELAY,
		DEL1_FEEDB,
		DEL1_BPM,
		REV1_WET,
		REV1_ROOM,
		REV1_DAMP,
		REV1_FEEDB,
		REV1_WIDTH,
		DYN1_COMPRESS,
		DYN1_LIMITER,

		KEY1_LOW,
		KEY1_HIGH,

		NUM_PARAMS
	};

	void setParamPort(ParamIndex index, float *pfParam);
	samplv1_port *paramPort(ParamIndex index) const;

	void setParamValue(ParamIndex index, float fValue);
	float paramValue(ParamIndex index) const;

	bool running(bool on);

	void stabilize();
	void reset();

	samplv1_controls *controls() const;
	samplv1_programs *programs() const;

	void process_midi(uint8_t *data, uint32_t size);
	void process(float **ins, float **outs, uint32_t nframes);

	void sampleOffsetLoopTest();

	virtual void updatePreset(bool bDirty) = 0;
	virtual void updateParam(ParamIndex index) = 0;
	virtual void updateParams() = 0;

	virtual void updateSample() = 0;

	virtual void updateOffsetRange() = 0;
	virtual void updateLoopRange() = 0;
	virtual void updateLoopFade() = 0;
	virtual void updateLoopZero() = 0;

	void midiInEnabled(bool on);
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

	virtual void updateTuning() = 0;

private:

	samplv1_impl *m_pImpl;
};


#endif// __samplv1_h

// end of samplv1.h
