// samplv1widget_jack.cpp
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

#include "samplv1widget_jack.h"

#include "samplv1_jack.h"

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
{
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
	initPreset();

	// Activate client...
	m_pSampl->activate();
}


// Destructor.
samplv1widget_jack::~samplv1widget_jack (void)
{
	m_pSampl->deactivate();
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


// Sample reset slot.
void samplv1widget_jack::clearSample (void)
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::clearSample()");
#endif
	m_pSampl->setSampleFile(0);

	updateSample(0);
}


// Sample loader slot.
void samplv1widget_jack::loadSample ( const QString& sFilename )
{
#ifdef CONFIG_DEBUG
	qDebug("samplv1widget_jack::loadSample(\"%s\")", sFilename.toUtf8().constData());
#endif
	m_pSampl->setSampleFile(sFilename.toUtf8().constData());

	updateSample(m_pSampl->sample());
}


// Sample filename retriever (crude experimental stuff III).
QString samplv1widget_jack::sampleFile (void) const
{
	return QString::fromUtf8(m_pSampl->sampleFile());
}


// Param method.
void samplv1widget_jack::updateParam (
	samplv1::ParamIndex index, float fValue ) const
{
	float *pParamPort = m_pSampl->paramPort(index);
	if (pParamPort)
		*pParamPort = fValue;
}


// Application close.
void samplv1widget_jack::closeEvent ( QCloseEvent *pCloseEvent )
{
	// Let's be sure about that...
	if (queryClose()) {
		pCloseEvent->accept();
	//	QApplication::quit();
	} else {
		pCloseEvent->ignore();
	}
}


//-------------------------------------------------------------------------
// main

int main( int argc, char *argv[] )
{
	Q_INIT_RESOURCE(samplv1);

	QApplication app(argc, argv);

	samplv1_jack sampl;
	samplv1widget_jack w(&sampl);
	if (argc > 1)
		w.loadPreset(argv[1]);
	w.show();

	return app.exec();
}


// end of samplv1widget_jack.cpp
