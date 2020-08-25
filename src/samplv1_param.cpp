// samplv1_param.cpp
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

#include "samplv1_param.h"
#include "samplv1_config.h"

#include <QHash>

#include <QDomDocument>
#include <QTextStream>
#include <QDir>

#include <math.h>


//-------------------------------------------------------------------------
// Abstract/absolute path functors.

QString samplv1_param::map_path::absolutePath (
	const QString& sAbstractPath ) const
{
	return QDir::current().absoluteFilePath(sAbstractPath);
}

QString samplv1_param::map_path::abstractPath (
	const QString& sAbsolutePath ) const
{
	return QDir::current().relativeFilePath(sAbsolutePath);
}


//-------------------------------------------------------------------------
// State params description.

enum ParamType { PARAM_FLOAT = 0, PARAM_INT, PARAM_BOOL };

static
struct ParamInfo {

	const char *name;
	ParamType type;
	float def;
	float min;
	float max;

} samplv1_params[samplv1::NUM_PARAMS] = {

	// name            type,           def,    min,    max
	{ "GEN1_SAMPLE",   PARAM_INT,    60.0f,   0.0f, 127.0f }, // GEN1 Sample
	{ "GEN1_REVERSE",  PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // GEN1 Reverse
	{ "GEN1_OFFSET",   PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // GEN1 Offset
	{ "GEN1_OFFSET_1", PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // GEN1 Offset Start
	{ "GEN1_OFFSET_2", PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // GEN1 Offset End
	{ "GEN1_LOOP",     PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // GEN1 Loop
	{ "GEN1_LOOP_1",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // GEN1 Loop Start
	{ "GEN1_LOOP_2",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // GEN1 Loop End
	{ "GEN1_OCTAVE",   PARAM_FLOAT,   0.0f,  -4.0f,   4.0f }, // GEN1 Octave
	{ "GEN1_TUNING",   PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // GEN1 Tuning
	{ "GEN1_GLIDE",    PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // GEN1 Glide
	{ "GEN1_ENVTIME",  PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // GEN1 Env.Time
	{ "DCF1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // DCF1 Enabled
	{ "DCF1_CUTOFF",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // DCF1 Cutoff
	{ "DCF1_RESO",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Resonance
	{ "DCF1_TYPE",     PARAM_INT,     0.0f,   0.0f,   3.0f }, // DCF1 Type
	{ "DCF1_SLOPE",    PARAM_INT,     0.0f,   0.0f,   3.0f }, // DCF1 Slope
	{ "DCF1_ENVELOPE", PARAM_FLOAT,   1.0f,  -1.0f,   1.0f }, // DCF1 Envelope
	{ "DCF1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCF1 Attack
	{ "DCF1_DECAY",    PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DCF1 Decay
	{ "DCF1_SUSTAIN",  PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Sustain
	{ "DCF1_RELEASE",  PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCF1 Release
	{ "LFO1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // LFO1 Enabled
	{ "LFO1_SHAPE",    PARAM_INT,     1.0f,   0.0f,   4.0f }, // LFO1 Wave Shape
	{ "LFO1_WIDTH",    PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // LFO1 Wave Width
	{ "LFO1_BPM",      PARAM_FLOAT, 180.0f,   0.0f, 360.0f }, // LFO1 BPM
	{ "LFO1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // LFO1 Rate
	{ "LFO1_SYNC",     PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // LFO1 Sync
	{ "LFO1_SWEEP",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Sweep
	{ "LFO1_PITCH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Pitch
	{ "LFO1_CUTOFF",   PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Cutoff
	{ "LFO1_RESO",     PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Resonance
	{ "LFO1_PANNING",  PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Panning
	{ "LFO1_VOLUME",   PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // LFO1 Volume
	{ "LFO1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // LFO1 Attack
	{ "LFO1_DECAY",    PARAM_FLOAT,   0.1f,   0.0f,   1.0f }, // LFO1 Decay
	{ "LFO1_SUSTAIN",  PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // LFO1 Sustain
	{ "LFO1_RELEASE",  PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // LFO1 Release
	{ "DCA1_ENABLED",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // DCA1 Enabled
	{ "DCA1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Volume
	{ "DCA1_ATTACK",   PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // DCA1 Attack
	{ "DCA1_DECAY",    PARAM_FLOAT,   0.1f,   0.0f,   1.0f }, // DCA1 Decay
	{ "DCA1_SUSTAIN",  PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // DCA1 Sustain
	{ "DCA1_RELEASE",  PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // DCA1 Release
	{ "OUT1_WIDTH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Stereo Width
	{ "OUT1_PANNING",  PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // OUT1 Panning
	{ "OUT1_FXSEND",   PARAM_FLOAT,   1.0f,   0.0f,   1.0f }, // OUT1 FX Send
	{ "OUT1_VOLUME",   PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // OUT1 Volume

	{ "DEF1_PITCHBEND",PARAM_FLOAT,   0.2f,   0.0f,   4.0f }, // DEF1 Pitchbend
	{ "DEF1_MODWHEEL", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Modwheel
	{ "DEF1_PRESSURE", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Pressure
	{ "DEF1_VELOCITY", PARAM_FLOAT,   0.2f,   0.0f,   1.0f }, // DEF1 Velocity
	{ "DEF1_CHANNEL",  PARAM_INT,     0.0f,   0.0f,  16.0f }, // DEF1 Channel
	{ "DEF1_MONO",     PARAM_INT,     0.0f,   0.0f,   2.0f }, // DEF1 Mono

	{ "CHO1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Chorus Wet
	{ "CHO1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Delay
	{ "CHO1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Feedback
	{ "CHO1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Rate
	{ "CHO1_MOD",      PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Chorus Modulation
	{ "FLA1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Flanger Wet
	{ "FLA1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Flanger Delay
	{ "FLA1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Flanger Feedback
	{ "FLA1_DAFT",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Flanger Daft
	{ "PHA1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Phaser Wet
	{ "PHA1_RATE",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Rate
	{ "PHA1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Feedback
	{ "PHA1_DEPTH",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Phaser Depth
	{ "PHA1_DAFT",     PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Phaser Daft
	{ "DEL1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Delay Wet
	{ "DEL1_DELAY",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Delay Delay
	{ "DEL1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Delay Feedback
	{ "DEL1_BPM",      PARAM_FLOAT, 180.0f,   0.0f, 360.0f }, // Delay BPM
	{ "REV1_WET",      PARAM_FLOAT,   0.0f,   0.0f,   1.0f }, // Reverb Wet
	{ "REV1_ROOM",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Room
	{ "REV1_DAMP",     PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Damp
	{ "REV1_FEEDB",    PARAM_FLOAT,   0.5f,   0.0f,   1.0f }, // Reverb Feedback
	{ "REV1_WIDTH",    PARAM_FLOAT,   0.0f,  -1.0f,   1.0f }, // Reverb Width
	{ "DYN1_COMPRESS", PARAM_BOOL,    0.0f,   0.0f,   1.0f }, // Dynamic Compressor
	{ "DYN1_LIMITER",  PARAM_BOOL,    1.0f,   0.0f,   1.0f }, // Dynamic Limiter

	{ "KEY1_LOW",      PARAM_INT,     0.0f,   0.0f, 127.0f }, // Keyboard Low
	{ "KEY1_HIGH",     PARAM_INT,   127.0f,   0.0f, 127.0f }  // Keyboard High
};


const char *samplv1_param::paramName ( samplv1::ParamIndex index )
{
	return samplv1_params[index].name;
}


float samplv1_param::paramDefaultValue ( samplv1::ParamIndex index )
{
	return samplv1_params[index].def;
}


float samplv1_param::paramSafeValue ( samplv1::ParamIndex index, float fValue )
{
	const ParamInfo& param = samplv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fValue > 0.5f ? 1.0f : 0.0f);

	if (fValue < param.min)
		return param.min;
	if (fValue > param.max)
		return param.max;

	if (param.type == PARAM_INT)
		return ::rintf(fValue);
	else
		return fValue;
}


float samplv1_param::paramValue ( samplv1::ParamIndex index, float fScale )
{
	const ParamInfo& param = samplv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fScale > 0.5f ? 1.0f : 0.0f);

	const float fValue = param.min + fScale * (param.max - param.min);

	if (param.type == PARAM_INT)
		return ::rintf(fValue);
	else
		return fValue;
}


float samplv1_param::paramScale ( samplv1::ParamIndex index, float fValue )
{
	const ParamInfo& param = samplv1_params[index];

	if (param.type == PARAM_BOOL)
		return (fValue > 0.5f ? 1.0f : 0.0f);

	const float fScale = (fValue - param.min) / (param.max - param.min);

	if (param.type == PARAM_INT)
		return ::rintf(fScale);
	else
		return fScale;
}


bool samplv1_param::paramFloat ( samplv1::ParamIndex index )
{
	return (samplv1_params[index].type == PARAM_FLOAT);
}


// Preset serialization methods.
bool samplv1_param::loadPreset (
	samplv1 *pSampl, const QString& sFilename )
{
	if (pSampl == nullptr)
		return false;

	QFileInfo fi(sFilename);
	if (!fi.exists()) {
		samplv1_config *pConfig = samplv1_config::getInstance();
		if (pConfig) {
			const QString& sPresetFile
				= pConfig->presetFile(sFilename);
			if (sPresetFile.isEmpty())
				return false;
			fi.setFile(sPresetFile);
			if (!fi.exists())
				return false;
		}
	}

	QFile file(fi.filePath());
	if (!file.open(QIODevice::ReadOnly))
		return false;

	const bool running = pSampl->running(false);

	pSampl->setTuningEnabled(false);
	pSampl->reset();

	static QHash<QString, samplv1::ParamIndex> s_hash;
	if (s_hash.isEmpty()) {
		for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
			const samplv1::ParamIndex index = samplv1::ParamIndex(i);
			s_hash.insert(samplv1_param::paramName(index), index);
		}
	}

	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(SAMPLV1_TITLE);
	if (doc.setContent(&file)) {
		QDomElement ePreset = doc.documentElement();
		if (ePreset.tagName() == "preset") {
		//	&& ePreset.attribute("name") == fi.completeBaseName()) {
			for (QDomNode nChild = ePreset.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "params") {
					for (QDomNode nParam = eChild.firstChild();
							!nParam.isNull();
								nParam = nParam.nextSibling()) {
						QDomElement eParam = nParam.toElement();
						if (eParam.isNull())
							continue;
						if (eParam.tagName() == "param") {
							samplv1::ParamIndex index = samplv1::ParamIndex(
								eParam.attribute("index").toULong());
							const QString& sName = eParam.attribute("name");
							if (!sName.isEmpty()) {
								if (!s_hash.contains(sName))
									continue;
								index = s_hash.value(sName);
							}
							const float fValue = eParam.text().toFloat();
							pSampl->setParamValue(index,
								samplv1_param::paramSafeValue(index, fValue));
						}
					}
				}
				else
				if (eChild.tagName() == "samples") {
					samplv1_param::loadSamples(pSampl, eChild);
				}
				else
				if (eChild.tagName() == "tuning") {
					samplv1_param::loadTuning(pSampl, eChild);
				}
			}
		}
	}

	file.close();

	pSampl->stabilize();
	pSampl->reset();
	pSampl->running(running);

	QDir::setCurrent(currentDir.absolutePath());

	return true;
}


bool samplv1_param::savePreset (
	samplv1 *pSampl, const QString& sFilename, bool bSymLink )
{
	if (pSampl == nullptr)
		return false;

	pSampl->stabilize();

	const QFileInfo fi(sFilename);
	const QDir currentDir(QDir::current());
	QDir::setCurrent(fi.absolutePath());

	QDomDocument doc(SAMPLV1_TITLE);
	QDomElement ePreset = doc.createElement("preset");
	ePreset.setAttribute("name", fi.completeBaseName());
	ePreset.setAttribute("version", CONFIG_BUILD_VERSION);

	QDomElement eSamples = doc.createElement("samples");
	samplv1_param::saveSamples(pSampl, doc, eSamples, map_path(), bSymLink);
	ePreset.appendChild(eSamples);

	QDomElement eParams = doc.createElement("params");
	for (uint32_t i = 0; i < samplv1::NUM_PARAMS; ++i) {
		QDomElement eParam = doc.createElement("param");
		const samplv1::ParamIndex index = samplv1::ParamIndex(i);
		eParam.setAttribute("index", QString::number(i));
		eParam.setAttribute("name", samplv1_param::paramName(index));
		const float fValue = pSampl->paramValue(index);
		eParam.appendChild(doc.createTextNode(QString::number(fValue)));
		eParams.appendChild(eParam);
	}
	ePreset.appendChild(eParams);

	if (pSampl->isTuningEnabled()) {
		QDomElement eTuning = doc.createElement("tuning");
		samplv1_param::saveTuning(pSampl, doc, eTuning, bSymLink);
		ePreset.appendChild(eTuning);
	}

	doc.appendChild(ePreset);

	QFile file(fi.filePath());
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	QTextStream(&file) << doc.toString();
	file.close();

	QDir::setCurrent(currentDir.absolutePath());

	return true;
}


// Sample serialization methods.
void samplv1_param::loadSamples (
	samplv1 *pSampl, const QDomElement& eSamples,
	const samplv1_param::map_path& mapPath )
{
	if (pSampl == nullptr)
		return;

	for (QDomNode nSample = eSamples.firstChild();
			!nSample.isNull();
				nSample = nSample.nextSibling()) {
		QDomElement eSample = nSample.toElement();
		if (eSample.isNull())
			continue;
		if (eSample.tagName() == "sample") {
		//	int index = eSample.attribute("index").toInt();
			QString sSampleFile;
			uint16_t iOctaves = 0;
			uint32_t iOffsetStart = 0;
			uint32_t iOffsetEnd = 0;
			uint32_t iLoopStart = 0;
			uint32_t iLoopEnd = 0;
			uint32_t iLoopFade = 0;
			bool bLoopZero = true;
			for (QDomNode nChild = eSample.firstChild();
					!nChild.isNull();
						nChild = nChild.nextSibling()) {
				QDomElement eChild = nChild.toElement();
				if (eChild.isNull())
					continue;
				if (eChild.tagName() == "filename") {
					sSampleFile = eChild.text();
				}
				else
				if (eChild.tagName() == "octaves") {
					iOctaves = eChild.text().toUInt();
				}
				else
				if (eChild.tagName() == "offset-start") {
					iOffsetStart = eChild.text().toULong();
				}
				else
				if (eChild.tagName() == "offset-end") {
					iOffsetEnd = eChild.text().toULong();
				}
				else
				if (eChild.tagName() == "loop-start") {
					iLoopStart = eChild.text().toULong();
				}
				else
				if (eChild.tagName() == "loop-end") {
					iLoopEnd = eChild.text().toULong();
				}
				else
				if (eChild.tagName() == "loop-fade") {
					iLoopFade = eChild.text().toULong();
				}
				else
				if (eChild.tagName() == "loop-zero") {
					bLoopZero = (eChild.text().toInt() > 0);
				}
			}
			// Legacy loader...
			if (sSampleFile.isEmpty())
				sSampleFile = eSample.text();
			// Done it.
			const QByteArray aSampleFile
				= mapPath.absolutePath(
					samplv1_param::loadFilename(sSampleFile)).toUtf8();
			pSampl->setSampleFile(aSampleFile.constData(), iOctaves);
			// Set actual sample loop points...
			pSampl->setLoopZero(bLoopZero);
			pSampl->setLoopFade(iLoopFade);
			pSampl->setLoopRange(iLoopStart, iLoopEnd);
			pSampl->setOffsetRange(iOffsetStart, iOffsetEnd);
		}
	}

	// Consolidate sample state...
	pSampl->updateSample();
}


void samplv1_param::saveSamples (
	samplv1 *pSampl, QDomDocument& doc, QDomElement& eSamples,
	const samplv1_param::map_path& mapPath, bool bSymLink )
{
	if (pSampl == nullptr)
		return;

	const char *pszSampleFile = pSampl->sampleFile();
	if (pszSampleFile == nullptr)
		return;

	QDomElement eSample = doc.createElement("sample");
	eSample.setAttribute("index", 0);
	eSample.setAttribute("name", "GEN1_SAMPLE");

	QDomElement eFilename = doc.createElement("filename");
	eFilename.appendChild(doc.createTextNode(mapPath.abstractPath(
		samplv1_param::saveFilename(
			QString::fromUtf8(pszSampleFile), bSymLink))));
	eSample.appendChild(eFilename);

	const uint16_t iOctaves = pSampl->octaves();
	if (iOctaves > 0) {
		QDomElement eOctaves = doc.createElement("octaves");
		eOctaves.appendChild(doc.createTextNode(
			QString::number(iOctaves)));
		eSample.appendChild(eOctaves);
	}

	const uint32_t iOffsetStart = pSampl->offsetStart();
	const uint32_t iOffsetEnd   = pSampl->offsetEnd();
	if (iOffsetStart < iOffsetEnd) {
		QDomElement eOffsetStart = doc.createElement("offset-start");
		eOffsetStart.appendChild(doc.createTextNode(
			QString::number(iOffsetStart)));
		eSample.appendChild(eOffsetStart);
		QDomElement eOffsetEnd = doc.createElement("offset-end");
		eOffsetEnd.appendChild(doc.createTextNode(
			QString::number(iOffsetEnd)));
		eSample.appendChild(eOffsetEnd);
	}

	const uint32_t iLoopStart = pSampl->loopStart();
	const uint32_t iLoopEnd   = pSampl->loopEnd();
	const uint32_t iLoopFade  = pSampl->loopFade();
	const bool     bLoopZero  = pSampl->isLoopZero();
	if (iLoopStart < iLoopEnd) {
		QDomElement eLoopStart = doc.createElement("loop-start");
		eLoopStart.appendChild(doc.createTextNode(
			QString::number(iLoopStart)));
		eSample.appendChild(eLoopStart);
		QDomElement eLoopEnd = doc.createElement("loop-end");
		eLoopEnd.appendChild(doc.createTextNode(
			QString::number(iLoopEnd)));
		eSample.appendChild(eLoopEnd);
		QDomElement eLoopFade = doc.createElement("loop-fade");
		eLoopFade.appendChild(doc.createTextNode(
			QString::number(iLoopFade)));
		eSample.appendChild(eLoopFade);
		QDomElement eLoopZero = doc.createElement("loop-zero");
		eLoopZero.appendChild(doc.createTextNode(
			QString::number(int(bLoopZero))));
		eSample.appendChild(eLoopZero);
	}

	eSamples.appendChild(eSample);
}


// Tuning serialization methods.
void samplv1_param::loadTuning (
	samplv1 *pSampl, const QDomElement& eTuning )
{
	if (pSampl == nullptr)
		return;

	pSampl->setTuningEnabled(eTuning.attribute("enabled").toInt() > 0);

	for (QDomNode nChild = eTuning.firstChild();
			!nChild.isNull();
				nChild = nChild.nextSibling()) {
		QDomElement eChild = nChild.toElement();
		if (eChild.isNull())
			continue;
		if (eChild.tagName() == "enabled") {
			pSampl->setTuningEnabled(eChild.text().toInt() > 0);
		}
		if (eChild.tagName() == "ref-pitch") {
			pSampl->setTuningRefPitch(eChild.text().toFloat());
		}
		else
		if (eChild.tagName() == "ref-note") {
			pSampl->setTuningRefNote(eChild.text().toInt());
		}
		else
		if (eChild.tagName() == "scale-file") {
			const QString& sScaleFile
				= eChild.text();
			const QByteArray aScaleFile
				= samplv1_param::loadFilename(sScaleFile).toUtf8();
			pSampl->setTuningScaleFile(aScaleFile.constData());
		}
		else
		if (eChild.tagName() == "keymap-file") {
			const QString& sKeyMapFile
				= eChild.text();
			const QByteArray aKeyMapFile
				= samplv1_param::loadFilename(sKeyMapFile).toUtf8();
			pSampl->setTuningScaleFile(aKeyMapFile.constData());
		}
	}

	// Consolidate tuning state...
	pSampl->updateTuning();
}


void samplv1_param::saveTuning (
	samplv1 *pSampl, QDomDocument& doc, QDomElement& eTuning, bool bSymLink )
{
	if (pSampl == nullptr)
		return;

	eTuning.setAttribute("enabled", int(pSampl->isTuningEnabled()));

	QDomElement eRefPitch = doc.createElement("ref-pitch");
	eRefPitch.appendChild(doc.createTextNode(
		QString::number(pSampl->tuningRefPitch())));
	eTuning.appendChild(eRefPitch);

	QDomElement eRefNote = doc.createElement("ref-note");
	eRefNote.appendChild(doc.createTextNode(
		QString::number(pSampl->tuningRefNote())));
	eTuning.appendChild(eRefNote);

	const char *pszScaleFile = pSampl->tuningScaleFile();
	if (pszScaleFile) {
		const QString& sScaleFile
			= QString::fromUtf8(pszScaleFile);
		if (!sScaleFile.isEmpty()) {
			QDomElement eScaleFile = doc.createElement("scale-file");
			eScaleFile.appendChild(doc.createTextNode(
				QDir::current().relativeFilePath(
					samplv1_param::saveFilename(sScaleFile, bSymLink))));
			eTuning.appendChild(eScaleFile);
		}
	}

	const char *pszKeyMapFile = pSampl->tuningKeyMapFile();
	if (pszKeyMapFile) {
		const QString& sKeyMapFile
			= QString::fromUtf8(pszKeyMapFile);
		if (!sKeyMapFile.isEmpty()) {
			QDomElement eKeyMapFile = doc.createElement("keymap-file");
			eKeyMapFile.appendChild(doc.createTextNode(
				QDir::current().relativeFilePath(
					samplv1_param::saveFilename(sKeyMapFile, bSymLink))));
			eTuning.appendChild(eKeyMapFile);
		}
	}
}


// Load/save and convert canonical/absolute filename helpers.
QString samplv1_param::loadFilename ( const QString& sFilename )
{
	QFileInfo fi(sFilename);
	if (fi.isSymLink())
		fi.setFile(fi.symLinkTarget());
	return fi.filePath();
}


QString samplv1_param::saveFilename ( const QString& sFilename, bool bSymLink )
{
	QFileInfo fi(sFilename);
	if (bSymLink && fi.absolutePath() != QDir::current().absolutePath()) {
		const QString& sPath = fi.absoluteFilePath();
		const QString& sName = fi.baseName();
		const QString& sExt  = fi.completeSuffix();
		const QString& sLink = sName
			+ '-' + QString::number(qHash(sPath), 16)
			+ '.' + sExt;
		QFile(sPath).link(sLink);
		fi.setFile(QDir::current(), sLink);
	}
	else if (fi.isSymLink()) fi.setFile(fi.symLinkTarget());
	return fi.absoluteFilePath();
}


// end of samplv1_param.cpp
