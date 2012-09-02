// samplv1widget_lv2.h
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

#ifndef __samplv1widget_lv2_h
#define __samplv1widget_lv2_h

#include "samplv1widget.h"

#include "lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"


#define SAMPLV1_LV2UI_URI SAMPLV1_LV2_PREFIX "ui"


// Forward decls.
class samplv1_lv2;

class QSocketNotifier;


//-------------------------------------------------------------------------
// samplv1widget_lv2 - decl.
//

class samplv1widget_lv2 : public samplv1widget
{
	Q_OBJECT

public:

	// Constructor.
	samplv1widget_lv2(samplv1_lv2 *pSampl,
		LV2UI_Controller controller, LV2UI_Write_Function write_function);

	// Destructor.
	~samplv1widget_lv2();

	// Plugin port event notification.
	void port_event(uint32_t port_index,
		uint32_t buffer_size, uint32_t format, const void *buffer);

protected slots:

	// Sample clear slot.
	void clearSample();

	// Sample loader slot.
	void loadSample(const QString& sFilename);

	// Update notification slot.
	void updateNotify();

protected:

	// Param methods.
	void updateParam(samplv1::ParamIndex index, float fValue) const;

	// Sample filename retriever (crude experimental stuff III).
	QString sampleFile() const;

private:

	// Instance variables.
	samplv1_lv2 *m_pSampl;

	LV2UI_Controller     m_controller;
	LV2UI_Write_Function m_write_function;

	// Update notifier.
	QSocketNotifier *m_pUpdateNotifier;
};


#endif	// __samplv1widget_lv2_h

// end of samplv1widget_lv2.h
