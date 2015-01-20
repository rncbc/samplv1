// samplv1_param.h
//
/****************************************************************************
   Copyright (C) 2012-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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
	// Sample serialization methods.
	void loadSamples(samplv1 *pSampl,
		const QDomElement& eSamples);
	void saveSamples(samplv1 *pSampl,
		QDomDocument& doc, QDomElement& eSamples);

	// Preset serialization methods.
	void loadPreset(samplv1 *pSampl,
		const QString& sFilename);
	void savePreset(samplv1 *pSampl,
		const QString& sFilename);

	// Default parameter name/value helpers.
	const char *paramName(samplv1::ParamIndex index);
	float paramDefaultValue(samplv1::ParamIndex index);
};


#endif	// __samplv1_param_h

// end of samplv1_param.h
