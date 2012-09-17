// samplv1widget_jack.h
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

#ifndef __samplv1widget_jack_h
#define __samplv1widget_jack_h

#include "samplv1widget.h"


// Forward decls.
class samplv1_jack;


//-------------------------------------------------------------------------
// samplv1widget_jack - decl.
//

class samplv1widget_jack : public samplv1widget
{
	Q_OBJECT

public:

	// Constructor.
	samplv1widget_jack(samplv1_jack *pSampl);

	// Destructor.
	~samplv1widget_jack();

#ifdef CONFIG_JACK_SESSION

	// JACK session self-notification.
	void notifySessionEvent(void *pvSessionArg);

signals:

	// JACK session notify event.
	void sessionNotify(void *);

protected slots:

	// JACK session event handler.
	void sessionEvent(void *pvSessionArg);

#endif	// CONFIG_JACK_SESSION

protected slots:

	// Sample clear slot.
	void clearSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

protected:

	// Param methods.
	void updateParam(samplv1::ParamIndex index, float fValue) const;

	// Sample filename retriever (crude experimental stuff III).
	QString sampleFile() const;

	// Application close.
	void closeEvent(QCloseEvent *pCloseEvent);

private:

	// Instance variables.
	samplv1_jack *m_pSampl;
};


#endif	// __samplv1widget_jack_h

// end of samplv1widget_jack.h
