// samplv1widget_jack.cpp
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

#include "samplv1widget_jack.h"

#include "samplv1widget_palette.h"

#include "samplv1_jack.h"

#ifdef CONFIG_NSM
#include "samplv1_nsm.h"
#endif

#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <QCloseEvent>

#include <QStyleFactory>

#ifndef CONFIG_LIBDIR
#if defined(__x86_64__)
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib64"
#else
#define CONFIG_LIBDIR CONFIG_PREFIX "/lib"
#endif
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt4/plugins"
#else
#define CONFIG_PLUGINSDIR CONFIG_LIBDIR "/qt5/plugins"
#endif


//-------------------------------------------------------------------------
// samplv1widget_jack - impl.
//

// Constructor.
samplv1widget_jack::samplv1widget_jack ( samplv1_jack *pSampl )
	: samplv1widget(), m_pSampl(pSampl)
	#ifdef CONFIG_NSM
		, m_pNsmClient(nullptr)
	#endif
{
	// Special style paths...
	if (QDir(CONFIG_PLUGINSDIR).exists())
		QApplication::addLibraryPath(CONFIG_PLUGINSDIR);

	// Custom color/style themes...
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig) {
		if (!pConfig->sCustomColorTheme.isEmpty()) {
			QPalette pal;
			if (samplv1widget_palette::namedPalette(
					pConfig, pConfig->sCustomColorTheme, pal))
				QApplication::setPalette(pal);
		}
		if (!pConfig->sCustomStyleTheme.isEmpty()) {
			QApplication::setStyle(
				QStyleFactory::create(pConfig->sCustomStyleTheme));
		}
	}

	// Initialize (user) interface stuff...
	m_pSamplUi = new samplv1_ui(m_pSampl, false);

	// Initialise preset stuff...
	clearPreset();

	// Initial update, always...
	updateSample(m_pSamplUi->sample());

	resetParamValues();
	resetParamKnobs();

	// May initialize the scheduler/work notifier.
	openSchedNotifier();
}


// Destructor.
samplv1widget_jack::~samplv1widget_jack (void)
{
	delete m_pSamplUi;
}


// Synth engine accessor.
samplv1_ui *samplv1widget_jack::ui_instance (void) const
{
	return m_pSamplUi;
}

#ifdef CONFIG_NSM

// NSM client accessors.
void samplv1widget_jack::setNsmClient ( samplv1_nsm *pNsmClient )
{
	m_pNsmClient = pNsmClient;
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
	m_pSamplUi->setParamValue(index, fValue);
}


// Dirty flag method.
void samplv1widget_jack::updateDirtyPreset ( bool bDirtyPreset )
{
	samplv1widget::updateDirtyPreset(bDirtyPreset);

#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active())
		m_pNsmClient->dirty(true); // as far as NSM goes, we're always filthy!
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
	#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		QApplication::exit(0);
	#else
		QApplication::quit();
	#endif
	} else {
		pCloseEvent->ignore();
	}
}


#ifdef CONFIG_NSM

// Optional GUI handlers.
void samplv1widget_jack::showEvent ( QShowEvent *pShowEvent )
{
	samplv1widget::showEvent(pShowEvent);

	if (m_pNsmClient)
		m_pNsmClient->visible(true);
}

void samplv1widget_jack::hideEvent ( QHideEvent *pHideEvent )
{
	if (m_pNsmClient)
		m_pNsmClient->visible(false);

	samplv1widget::hideEvent(pHideEvent);
}

#endif	// CONFIG_NSM


// end of samplv1widget_jack.cpp
