// samplv1widget_jack.cpp
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

#include "samplv1widget_jack.h"

#include "samplv1_jack.h"

#ifdef CONFIG_NSM
#include "samplv1_nsm.h"
#endif

#include <QFileInfo>
#include <QDir>

#include <QCloseEvent>


//-------------------------------------------------------------------------
// samplv1widget_jack - impl.
//

// Constructor.
samplv1widget_jack::samplv1widget_jack ( samplv1_jack *pSampl )
	: samplv1widget(), m_pSampl(pSampl)
	#ifdef CONFIG_NSM
		, m_pNsmClient(NULL)
	#endif
{
	// Initialize preset stuff...
	// initPreset();
	updateSample(m_pSampl->sample());
	updateParamValues();
}


// Synth engine accessor.
samplv1 *samplv1widget_jack::instance (void) const
{
	return m_pSampl;
}

#ifdef CONFIG_NSM

// NSM client accessors.
void samplv1widget_jack::setNsmClient ( samplv1_nsm *pNsmClient )
{
	m_pNsmClient = pNsmClient;

	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig)
		pConfig->bDontUseNativeDialogs = true;
}

samplv1_nsm *samplv1widget_jack::nsmClient (void) const
{
	return m_pNsmClient;
}

#endif	// CONFIG_NSM


// Param port method.
void samplv1widget_jack::updateParam (
	samplv1::ParamIndex index, float fValue ) const
{
	m_pSampl->setParamValue(index, fValue);
}


// Dirty flag method.
void samplv1widget_jack::updateDirtyPreset ( bool bDirtyPreset )
{
	samplv1widget::updateDirtyPreset(bDirtyPreset);

#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active())
		m_pNsmClient->dirty(bDirtyPreset);
#endif
}


// Application close.
void samplv1widget_jack::closeEvent ( QCloseEvent *pCloseEvent )
{
#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active())
		samplv1widget::updateDirtyPreset(false);
#endif

	// Let's be sure about that...
	if (queryClose()) {
		pCloseEvent->accept();
	//	QApplication::quit();
	} else {
		pCloseEvent->ignore();
	}
}


#ifdef CONFIG_NSM

// Optional GUI handlers.
void samplv1widget_jack::showEvent ( QShowEvent *pShowEvent )
{
	QWidget::showEvent(pShowEvent);

	if (m_pNsmClient)
		m_pNsmClient->visible(true);
}

void samplv1widget_jack::hideEvent ( QHideEvent *pHideEvent )
{
	if (m_pNsmClient)
		m_pNsmClient->visible(false);

	QWidget::hideEvent(pHideEvent);
}

#endif	// CONFIG_NSM


// end of samplv1widget_jack.cpp
