// samplv1_param.h
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

#ifndef __samplv1_param_h
#define __samplv1_param_h

#include "samplv1.h"

#include <QString>

// forward decl.
class QDomElement;
class QDomDocument;


//-------------------------------------------------------------------------
// samplv1_param - decl.
//

namespace samplv1_param
{
	// Abstract/absolute path functors.
	class map_path
	{
	public:

		virtual QString absolutePath(const QString& sAbstractPath) const;
		virtual QString abstractPath(const QString& sAbsolutePath) const;
	};

	// Preset serialization methods.
	bool loadPreset(samplv1 *pSampl,
		const QString& sFilename);
	bool savePreset(samplv1 *pSampl,
		const QString& sFilename,
		bool bSymLink = false);

	// Sample serialization methods.
	void loadSamples(samplv1 *pSampl,
		const QDomElement& eSamples,
		const map_path& mapPath = map_path());
	void saveSamples(samplv1 *pSampl,
		QDomDocument& doc, QDomElement& eSamples,
		const map_path& mapPath = map_path(),
		bool bSymLink = false);

	// Tuning serialization methods.
	void loadTuning(samplv1 *pSampl,
		const QDomElement& eTuning);
	void saveTuning(samplv1 *pSampl,
		QDomDocument& doc, QDomElement& eTuning,
		bool bSymLink = false);

	// Default parameter name/value helpers.
	const char *paramName(samplv1::ParamIndex index);
	float paramDefaultValue(samplv1::ParamIndex index);
	float paramSafeValue(samplv1::ParamIndex index, float fValue);
	float paramValue(samplv1::ParamIndex index, float fScale);
	float paramScale(samplv1::ParamIndex index, float fValue);
	bool paramFloat(samplv1::ParamIndex index);

	// Load/save and convert canonical/absolute filename helpers.
	QString loadFilename(const QString& sFilename);
	QString saveFilename(const QString& sFilename, bool bSymLink);
};


#endif	// __samplv1_param_h

// end of samplv1_param.h
