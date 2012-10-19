// samplv1widget_sample.cpp
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

#include "samplv1widget_sample.h"

#include "samplv1widget_config.h"

#include "samplv1_sample.h"

#include <QPainter>

#include <QFileDialog>
#include <QUrl>


//----------------------------------------------------------------------------
// samplv1widget_sample -- Custom widget

// Constructor.
samplv1widget_sample::samplv1widget_sample (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QFrame(pParent, wflags), m_pSample(0), m_iChannels(0), m_ppPolyg(0)
{
//	setMouseTracking(true);
	setMinimumSize(QSize(520, 80));

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);
}


// Destructor.
samplv1widget_sample::~samplv1widget_sample (void)
{
	setSample(0);
}


// Parameter accessors.
void samplv1widget_sample::setSample ( samplv1_sample *pSample )
{
	if (m_pSample && m_ppPolyg) {
		for (unsigned short k = 0; k < m_iChannels; ++k)
			delete m_ppPolyg[k];
		delete [] m_ppPolyg;
		m_ppPolyg = 0;
		m_iChannels = 0;
	}

	m_pSample = pSample;

	if (m_pSample)
		m_iChannels = m_pSample->channels();
	if (m_iChannels > 0 && m_ppPolyg == 0) {
		const int h = height();
		const int w = width() & 0x7ffe; // force even.
		const int w2 = (w >> 1);
		const unsigned int nframes = m_pSample->length();
		const unsigned int nperiod = nframes / w2;
		const int h0 = h / m_iChannels;
		const float h1 = float(h0 >> 1);
		int y0 = h1;
		m_ppPolyg = new QPolygon* [m_iChannels];
		for (unsigned short k = 0; k < m_iChannels; ++k) {
			m_ppPolyg[k] = new QPolygon(w);
			const float *pframes = m_pSample->frames(k);
			float vmax = 0.0f;
			float vmin = 0.0f;
			int n = 0;
			int x = 1;
			unsigned int j = nperiod;
			for (unsigned int i = 0; i < nframes; ++i) {
				const float v = *pframes++;
				if (vmax < v)
					vmax = v;
				if (vmin > v)
					vmin = v;
				if (++j > nperiod) {
					m_ppPolyg[k]->setPoint(n, x, y0 - int(vmax * h1));
					m_ppPolyg[k]->setPoint(w - n - 1, x, y0 - int(vmin * h1));
					j = 0; vmax = vmin = 0.0f;
					++n; x += 2;
				}
			}
			while (n < w2) {
				m_ppPolyg[k]->setPoint(n, x, y0);
				m_ppPolyg[k]->setPoint(w - n - 1, x, y0);
				++n; x += 2;
			}
			y0 += h0;
		}
	}

	QString sToolTip;
	const char *pszSampleFile = (m_pSample ? m_pSample->filename() : 0);
	if (pszSampleFile) {
		sToolTip += tr("%1\n%2 frames, %3 channels, %4 Hz")
			.arg(QFileInfo(pszSampleFile).completeBaseName())
			.arg(m_pSample->length())
			.arg(m_pSample->channels())
			.arg(m_pSample->rate());
	}
	setToolTip(sToolTip);

	update();
}

samplv1_sample *samplv1widget_sample::sample (void) const
{
	return m_pSample;
}


// Widget resize handler.
void samplv1widget_sample::resizeEvent ( QResizeEvent * )
{
//	setSample(m_pSample);	-- reset polygon...
}


// Mouse interaction.
void samplv1widget_sample::mouseDoubleClickEvent ( QMouseEvent */*pMouseEvent*/ )
{
	openSample();
}


// Draw curve.
void samplv1widget_sample::paintEvent ( QPaintEvent *pPaintEvent )
{
	QPainter painter(this);

	const QRect& rect = QWidget::rect();
	const int h = rect.height();
	const int w = rect.width();

	const QPalette& pal = palette();
	const bool bDark = (pal.window().color().value() < 0x7f);
	const QColor& rgbLite = (isEnabled()
		? (bDark ? Qt::darkYellow : Qt::yellow) : pal.mid().color());

	painter.fillRect(rect, pal.dark().color());

	if (m_pSample && m_ppPolyg) {
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(bDark ? Qt::gray : Qt::darkGray);
		QLinearGradient grad(0, 0, w << 1, h << 1);
		grad.setColorAt(0.0f, rgbLite);
		grad.setColorAt(1.0f, Qt::black);
		painter.setBrush(grad);
		for (unsigned short k = 0; k < m_iChannels; ++k)
			painter.drawPolygon(*m_ppPolyg[k]);
		painter.setRenderHint(QPainter::Antialiasing, false);
	} else {
		painter.setPen(pal.midlight().color());
		painter.drawText(rect, Qt::AlignCenter,
			tr("(double-click to load new sample...)"));
	}

	painter.end();

	QFrame::paintEvent(pPaintEvent);
}


// Browse for a new sample.
void samplv1widget_sample::openSample (void)
{
	samplv1widget_config *pConfig = samplv1widget_config::getInstance();
	if (pConfig == NULL)
		return;

	QString sFilename;

	// Cache supported file-types stuff from libsndfile...
	static QStringList s_filters;
	if (s_filters.isEmpty()) {
		const QString sExtMask("*.%1");
		const QString sFilterMask("%1 (%2)");
		QStringList exts;
		SF_FORMAT_INFO sffinfo;
		int iCount = 0;
		::sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &iCount, sizeof(int));
		for (int i = 0 ; i < iCount; ++i) {
			sffinfo.format = i;
			::sf_command(NULL, SFC_GET_FORMAT_MAJOR, &sffinfo, sizeof(sffinfo));
			const QString sName = QString(sffinfo.name)
				.replace('/', '-') // Replace some illegal characters.
				.remove('(').remove(')');
			const QString sExt = sExtMask.arg(sffinfo.extension);
			s_filters.append(sFilterMask.arg(sName).arg(sExt));
			exts.append(sExt);
		}
		s_filters.prepend(sFilterMask.arg(tr("Audio files")).arg(exts.join(" ")));
		s_filters.append(sFilterMask.arg(tr("All files")).arg("*.*"));
	}

	const QString& sTitle  = tr("Open Sample") + " - " SAMPLV1_TITLE;
	const QString& sFilter = s_filters.join(";;");
#if 1//QT_VERSION < 0x040400
	sFilename = QFileDialog::getOpenFileName(parentWidget(),
		sTitle, pConfig->sSampleDir, sFilter);
#else
	QFileDialog fileDialog(nativeParentWidget(),
		sTitle, pConfig->sSampleDir, sFilter);
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	QList<QUrl> urls(fileDialog.sidebarUrls());
	urls.append(QUrl::fromLocalFile(pConfig->sSampleDir));
	fileDialog.setSidebarUrls(urls);
	if (fileDialog.exec())
		sFilename = fileDialog.selectedFiles().first();
#endif
	if (!sFilename.isEmpty()) {
		pConfig->sSampleDir = QFileInfo(sFilename).absolutePath();
		emit loadSampleFile(sFilename);
	}
}


// Effective sample slot.
void samplv1widget_sample::loadSample ( samplv1_sample *pSample )
{
	setSample(pSample);
}


// end of samplv1widget_sample.cpp
