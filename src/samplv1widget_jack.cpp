// samplv1widget_jack.cpp
//
/****************************************************************************
   Copyright (C) 2012-2013, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <QCloseEvent>


#ifdef CONFIG_JACK_SESSION

#include <jack/session.h>

//----------------------------------------------------------------------
// qtractorAudioEngine_session_event -- JACK session event callabck
//

static void samplv1widget_jack_session_event (
	jack_session_event_t *pSessionEvent, void *pvArg )
{
	samplv1widget_jack *pWidget
		= static_cast<samplv1widget_jack *> (pvArg);

	pWidget->notifySessionEvent(pSessionEvent);
}

#endif	// CONFIG_JACK_SESSION


//-------------------------------------------------------------------------
// samplv1widget_jack - impl.
//

// Constructor.
samplv1widget_jack::samplv1widget_jack ( samplv1_jack *pSampl )
	: samplv1widget(), m_pSampl(pSampl)
	#ifdef CONFIG_NSM
		, m_pNsmClient(NULL), m_bNsmDirty(false)
	#endif
{
#ifdef CONFIG_NSM
	// Check whether to participate into a NSM session...
	const QString& nsm_url
		= QString::fromLatin1(::getenv("NSM_URL"));
	if (!nsm_url.isEmpty()) {
		m_pNsmClient = new samplv1_nsm(nsm_url);
		QObject::connect(m_pNsmClient,
			SIGNAL(open()),
			SLOT(openSession()));
		QObject::connect(m_pNsmClient,
			SIGNAL(save()),
			SLOT(saveSession()));
		QObject::connect(m_pNsmClient,
			SIGNAL(show()),
			SLOT(showSession()));
		QObject::connect(m_pNsmClient,
			SIGNAL(hide()),
			SLOT(hideSession()));
		m_pNsmClient->announce(SAMPLV1_TITLE, ":switch:dirty:optional-gui:");
		return;
	}
#endif	// CONFIG_NSM

	m_pSampl->open(SAMPLV1_TITLE);

#ifdef CONFIG_JACK_SESSION
	// JACK session event callback...
	if (::jack_set_session_callback) {
		::jack_set_session_callback(m_pSampl->client(),
			samplv1widget_jack_session_event, this);
		QObject::connect(this,
			SIGNAL(sessionNotify(void *)),
			SLOT(sessionEvent(void *)));
	}
#endif

	// Initialize preset stuff...
	// initPreset();

	// Activate client...
	m_pSampl->activate();
}


// Destructor.
samplv1widget_jack::~samplv1widget_jack (void)
{
	m_pSampl->deactivate();
	m_pSampl->close();
}


#ifdef CONFIG_JACK_SESSION

// JACK session event handler.
void samplv1widget_jack::notifySessionEvent ( void *pvSessionArg )
{
	emit sessionNotify(pvSessionArg);
}

void samplv1widget_jack::sessionEvent ( void *pvSessionArg )
{
	jack_session_event_t *pJackSessionEvent
		= (jack_session_event_t *) pvSessionArg;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::sessionEvent()"
		" type=%d client_uuid=\"%s\" session_dir=\"%s\"",
		int(pJackSessionEvent->type),
		pJackSessionEvent->client_uuid,
		pJackSessionEvent->session_dir);
#endif

	bool bQuit = (pJackSessionEvent->type == JackSessionSaveAndQuit);

	const QString sSessionDir
		= QString::fromUtf8(pJackSessionEvent->session_dir);
	const QString sSessionName
		= QFileInfo(QFileInfo(sSessionDir).canonicalPath()).completeBaseName();
	const QString sSessionFile = sSessionName + '.' + SAMPLV1_TITLE;

	QStringList args;
	args << QApplication::applicationFilePath();
	args << QString("\"${SESSION_DIR}%1\"").arg(sSessionFile);

	savePreset(QFileInfo(sSessionDir, sSessionFile).absoluteFilePath());

	const QByteArray aCmdLine = args.join(" ").toUtf8();
	pJackSessionEvent->command_line = ::strdup(aCmdLine.constData());

	jack_session_reply(m_pSampl->client(), pJackSessionEvent);
	jack_session_event_free(pJackSessionEvent);

	if (bQuit)
		close();
}

#endif	// CONFIG_JACK_SESSION


// Synth engine accessor.
samplv1 *samplv1widget_jack::instance (void) const
{
	return m_pSampl;
}


#ifdef CONFIG_NSM

void samplv1widget_jack::openSession (void)
{
	if (m_pNsmClient == NULL)
		return;

	if (!m_pNsmClient->is_active())
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::openSession()");
#endif

	m_pSampl->deactivate();
	m_pSampl->close();

	const QString& path_name = m_pNsmClient->path_name();
	const QString& display_name = m_pNsmClient->display_name();
	const QString& client_id = m_pNsmClient->client_id();

	const QDir dir(path_name);
	if (!dir.exists())
		dir.mkpath(path_name);

	const QFileInfo fi(path_name, display_name + '.' + SAMPLV1_TITLE);
	if (fi.exists())
		loadPreset(fi.absoluteFilePath());

	m_pSampl->open(client_id.toUtf8().constData());
	m_pSampl->activate();

	m_bNsmDirty = false;

	m_pNsmClient->open_reply();
	m_pNsmClient->dirty(false);

	m_pNsmClient->visible(QWidget::isVisible());
}

void samplv1widget_jack::saveSession (void)
{
	if (m_pNsmClient == NULL)
		return;

	if (!m_pNsmClient->is_active())
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::saveSession()");
#endif

	if (m_bNsmDirty) {
		const QString& path_name = m_pNsmClient->path_name();
		const QString& display_name = m_pNsmClient->display_name();
	//	const QString& client_id = m_pNsmClient->client_id();
		const QFileInfo fi(path_name, display_name + '.' + SAMPLV1_TITLE);
		savePreset(fi.absoluteFilePath());
		m_bNsmDirty = false;
	}

	m_pNsmClient->save_reply();
	m_pNsmClient->dirty(false);
}


void samplv1widget_jack::showSession (void)
{
	if (m_pNsmClient == NULL)
		return;

	if (!m_pNsmClient->is_active())
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::showSession()");
#endif

	QWidget::show();
}

void samplv1widget_jack::hideSession (void)
{
	if (m_pNsmClient == NULL)
		return;

	if (!m_pNsmClient->is_active())
		return;

#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::hideSession()");
#endif

	QWidget::hide();
}


#endif	// CONFIG_NSM


// Param port method.
void samplv1widget_jack::updateParam (
	samplv1::ParamIndex index, float fValue ) const
{
	float *pParamPort = m_pSampl->paramPort(index);
	if (pParamPort)
		*pParamPort = fValue;
}


// Dirty flag method.
void samplv1widget_jack::updateDirtyPreset ( bool bDirtyPreset )
{
	samplv1widget::updateDirtyPreset(bDirtyPreset);

#ifdef CONFIG_NSM
	if (m_pNsmClient && m_pNsmClient->is_active()) {
		if (!m_bNsmDirty/* && bDirtyPreset*/) {
			m_pNsmClient->dirty(true);
			m_bNsmDirty = true;
		}
	}
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


//-------------------------------------------------------------------------
// main

#include <QTextStream>

static bool parse_args ( const QStringList& args )
{
	QTextStream out(stderr);

	QStringListIterator iter(args);
	while (iter.hasNext()) {
		const QString& sArg = iter.next();
		if (sArg == "-h" || sArg == "--help") {
			out << QObject::tr(
				"Usage: %1 [options] [preset-file]\n\n"
				SAMPLV1_TITLE " - " SAMPLV1_SUBTITLE "\n\n"
				"Options:\n\n"
				"  -h, --help\n\tShow help about command line options\n\n"
				"  -v, --version\n\tShow version information\n\n")
				.arg(args.at(0));
			return false;
		}
		else
		if (sArg == "-v" || sArg == "-V" || sArg == "--version") {
			out << QObject::tr("Qt: %1\n").arg(qVersion());
			out << QObject::tr(SAMPLV1_TITLE ": %1\n").arg(SAMPLV1_VERSION);
			return false;
		}
	}

	return true;
}


int main ( int argc, char *argv[] )
{
	Q_INIT_RESOURCE(samplv1);

	QApplication app(argc, argv);
	if (!parse_args(app.arguments())) {
		app.quit();
		return 1;
	}

	samplv1_jack sampl;
	samplv1widget_jack w(&sampl);
	if (argc > 1)
		w.loadPreset(argv[1]);
	else
		w.initPreset();
	w.show();

	return app.exec();
}


// end of samplv1widget_jack.cpp
