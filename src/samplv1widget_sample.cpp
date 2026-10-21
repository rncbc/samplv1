// samplv1widget_sample.cpp
//
/****************************************************************************
   Copyright (C) 2012-2026, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "samplv1_config.h"
#include "samplv1_sample.h"

#include "samplv1_ui.h"

#include "samplv1widget_spinbox.h"

#include <sndfile.h>

#include <QPainter>

#include <QLinearGradient>

#include <QApplication>
#include <QFileDialog>
#include <QMimeData>
#include <QDrag>
#include <QUrl>
#include <QTimer>

#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>

#include <QToolTip>


//----------------------------------------------------------------------------
// samplv1widget_sample -- Custom widget

// Constructor.
samplv1widget_sample::samplv1widget_sample ( QWidget *pParent )
	: QFrame(pParent), m_pSamplUi(nullptr),
		m_pSample(nullptr), m_iChannels(0), m_ppPolyg(nullptr)
{
	QFrame::setMouseTracking(true);
	QFrame::setFocusPolicy(Qt::ClickFocus);

	QFrame::setMinimumSize(QSize(480, 80));
	QFrame::setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	QFrame::setAcceptDrops(true);

	QFrame::setFrameShape(QFrame::Panel);
	QFrame::setFrameShadow(QFrame::Sunken);

	m_bOffset = false;
	m_iOffsetStart = m_iOffsetEnd = 0;

	m_bLoop = false;
	m_iLoopStart = m_iLoopEnd = 0;

	// Crop: Initialize the visual sample range.
	m_iViewStart = m_iViewEnd = 0;

	m_dragCursor  = DragNone;
	m_pDragSample = nullptr;

	m_iDirectNoteOn = -1;

	m_fEnvTime = 0.0f;

	m_fDca1Attack  = 0.0f;
	m_fDca1Decay   = 0.0f;
	m_fDca1Sustain = 0.0f;
	m_fDca1Release = 0.0f;

	resetDragState();
}


// Destructor.
samplv1widget_sample::~samplv1widget_sample (void)
{
	setSample(nullptr);
}


// Settlers.
void samplv1widget_sample::setInstance ( samplv1_ui *pSamplUi )
{
	m_pSamplUi = pSamplUi;
}


samplv1_ui *samplv1widget_sample::instance (void) const
{
	return m_pSamplUi;
}


// Parameter accessors.
void samplv1widget_sample::updateView (void)
{
	if (m_ppPolyg) {
		for (unsigned short k = 0; k < m_iChannels; ++k)
			delete m_ppPolyg[k];
		delete [] m_ppPolyg;
		m_ppPolyg = nullptr;
	}

	m_iChannels = (m_pSample ? m_pSample->channels() : 0);
	if (m_iChannels == 0)
		return;

	const int h = height();
	const int w = width() & 0x7ffe; // Force even.
	const int w2 = (w >> 1);
	const int h0 = h / m_iChannels;
	const int h1 = (h0 >> 1);
	const uint32_t nframes = m_pSample->length();

	uint32_t iViewStart = m_iViewStart;
	uint32_t iViewEnd = m_iViewEnd;

	if (iViewStart > nframes)
		iViewStart = nframes;
	if (iViewEnd > nframes)
		iViewEnd = nframes;
	if (iViewEnd < iViewStart)
		iViewEnd = iViewStart;

	m_ppPolyg = new QPolygon *[m_iChannels];

	int y0 = h1;
	for (unsigned short k = 0; k < m_iChannels; ++k) {
		m_ppPolyg[k] = new QPolygon(w);
		for (int bin = 0; bin < w2; ++bin) {
			const uint32_t iFrameStart = iViewStart
				+ (uint64_t(bin) * uint64_t(iViewEnd - iViewStart))
					/ uint64_t(w2);
			const uint32_t iFrameEnd = iViewStart
				+ (uint64_t(bin + 1) * uint64_t(iViewEnd - iViewStart))
					/ uint64_t(w2);
 			float vmax = 0.0f;
 			float vmin = 0.0f;
			bool bFirst = true;
			const float *pframes = m_pSample->frames(k);
			for (uint32_t i = iFrameStart; i < iFrameEnd; ++i) {
				const float v = pframes[i];
				if (bFirst || vmax < v)
					vmax = v;
				if (bFirst || vmin > v)
					vmin = v;
				bFirst = false;
			}
			const int x = 1 + (bin << 1);
			const int yMax = y0 - int(vmax * h1);
			const int yMin = y0 - int(vmin * h1);
			m_ppPolyg[k]->setPoint(bin, x, yMax);
			m_ppPolyg[k]->setPoint(w - bin - 1, x, yMin);
		}
		y0 += h0;
	}
}
// }

void samplv1widget_sample::setSample ( samplv1_sample *pSample )
{
	// Crop {
	if (m_pSample != pSample) {
		m_pSample = pSample;
		m_iViewStart = 0;
		m_iViewEnd = (m_pSample ? m_pSample->length() : 0);
	}

	m_pDragSample = nullptr;

	updateView();
	// }

	updateToolTip();
	update();
}


samplv1_sample *samplv1widget_sample::sample (void) const
{
	return m_pSample;
}


void samplv1widget_sample::setSampleName ( const QString& sName )
{
	m_sName = sName;

	updateToolTip();
}

const QString& samplv1widget_sample::sampleName (void) const
{
	return m_sName;
}


// Offset mode.
void samplv1widget_sample::setOffset ( bool bOffset )
{
	m_bOffset = bOffset;

	updateToolTip();
	update();
}

bool samplv1widget_sample::isOffset (void) const
{
	return m_bOffset;
}


void samplv1widget_sample::setOffsetStart ( uint32_t iOffsetStart )
{
	m_iOffsetStart = iOffsetStart;

	updateToolTip();
	update();
}

uint32_t samplv1widget_sample::offsetStart (void) const
{
	return m_iOffsetStart;
}


void samplv1widget_sample::setOffsetEnd ( uint32_t iOffsetEnd )
{
	m_iOffsetEnd = iOffsetEnd;

	updateToolTip();
	update();
}

uint32_t samplv1widget_sample::offsetEnd (void) const
{
	return m_iOffsetEnd;
}


// Loop mode.
void samplv1widget_sample::setLoop ( bool bLoop )
{
	m_bLoop = bLoop;

	updateToolTip();
	update();
}

bool samplv1widget_sample::isLoop (void) const
{
	return m_bLoop;
}


void samplv1widget_sample::setLoopStart ( uint32_t iLoopStart )
{
	m_iLoopStart = iLoopStart;

	updateToolTip();
	update();
}

uint32_t samplv1widget_sample::loopStart (void) const
{
	return m_iLoopStart;
}


void samplv1widget_sample::setLoopEnd ( uint32_t iLoopEnd )
{
	m_iLoopEnd = iLoopEnd;

	updateToolTip();
	update();
}

uint32_t samplv1widget_sample::loopEnd (void) const
{
	return m_iLoopEnd;
}


// Crop {
void samplv1widget_sample::cropView (void)
{
	if (m_pSample == nullptr)
		return;

	setViewRange(m_iOffsetStart, m_iOffsetEnd);
}


void samplv1widget_sample::uncropView (void)
{
	if (m_pSample == nullptr)
		return;

	setViewRange(0, m_pSample->length());
}


void samplv1widget_sample::setViewRange (
	uint32_t iViewStart, uint32_t iViewEnd )
{
	if (m_pSample == nullptr)
		return;

	const uint32_t nframes = m_pSample->length();

	if (iViewStart > nframes)
		iViewStart = nframes;
	if (iViewEnd > nframes)
		iViewEnd = nframes;

	if (iViewStart >= iViewEnd && nframes > 0)
		return;

	m_iViewStart = iViewStart;
	m_iViewEnd = iViewEnd;

	updateView();
	update();
}
// }

// Sanitizer helper.
int samplv1widget_sample::safeX ( int x ) const
{
	const int w = QFrame::width();
	// Crop {
	if (x < 0)
		return 0;
	if (x > w)
		return w;
	return x;
	// }

}


// Sanitized converters.
int samplv1widget_sample::pixelFromFrames ( uint32_t n ) const
{
	// Crop {
	if (m_pSample == nullptr)
		return 0;

	const int w = QFrame::width();
	if (w < 1)
		return 0;

	const uint32_t nframes = m_pSample->length();
	uint32_t iViewStart = m_iViewStart;
	uint32_t iViewEnd = m_iViewEnd;

	if (iViewStart > nframes)
		iViewStart = nframes;
	if (iViewEnd > nframes)
		iViewEnd = nframes;
	if (iViewStart >= iViewEnd)
		return 0;

	if (iViewStart >= n)
		return 0;
	if (n >= iViewEnd)
		return w;

	const uint32_t nview = iViewEnd - iViewStart;
	const uint32_t npos = n - iViewStart;

	return int((uint64_t(w) * uint64_t(npos)) / uint64_t(nview));
	// }
}


uint32_t samplv1widget_sample::framesFromPixel ( int x ) const
{
	// Crop {
	if (m_pSample == nullptr)
		return 0;

	const int w = QFrame::width();
	if (w <= 0)
		return m_iViewStart;

	x = safeX(x);

	const uint32_t nframes = m_pSample->length();
	uint32_t iViewStart = m_iViewStart;
	uint32_t iViewEnd = m_iViewEnd;

	if (iViewStart > nframes)
		iViewStart = nframes;
	if (iViewEnd > nframes)
		iViewEnd = nframes;
	if (iViewEnd <= iViewStart)
		return iViewStart;

	const uint32_t nview = iViewEnd - iViewStart;
	return iViewStart + uint32_t((uint64_t(x) * uint64_t(nview)) / uint64_t(w));
	// }
}


// Widget resize handler.
void samplv1widget_sample::resizeEvent ( QResizeEvent * )
{
	setSample(m_pSample);	// reset polygon...
}


// Mouse interaction.
void samplv1widget_sample::mousePressEvent ( QMouseEvent *pMouseEvent )
{
	if (pMouseEvent->button() == Qt::LeftButton) {
		if (m_dragCursor == DragNone) {
			m_dragState = DragStart;
			m_posDrag = pMouseEvent->pos();
		} else {
			if (m_bOffset) {
				m_iDragOffsetStartX = pixelFromFrames(m_iOffsetStart);
				m_iDragOffsetEndX   = pixelFromFrames(m_iOffsetEnd);
				m_dragState = m_dragCursor;
			}
			if (m_bLoop) {
				m_iDragLoopStartX = pixelFromFrames(m_iLoopStart);
				m_iDragLoopEndX   = pixelFromFrames(m_iLoopEnd);
				m_dragState = m_dragCursor;
			}
		}
	}

	QFrame::mousePressEvent(pMouseEvent);
}


void samplv1widget_sample::mouseMoveEvent ( QMouseEvent *pMouseEvent )
{
	const int x = pMouseEvent->pos().x();

	switch (m_dragState) {
	case DragNone:
		if (m_pSample) {
			const int dx = QApplication::startDragDistance();
			if (m_bLoop) {
				int x0 = pixelFromFrames(m_iLoopEnd);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragLoopEnd;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Loop end: %1")
							.arg(textFromValue(m_iLoopEnd)), this);
					break;
				}
				x0 = pixelFromFrames(m_iLoopStart);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragLoopStart;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Loop start: %1")
							.arg(textFromValue(m_iLoopStart)), this);
					break;
				}
			}
			if (m_bOffset) {
				int x0 = pixelFromFrames(m_iOffsetEnd);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragOffsetEnd;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Offset end: %1")
							.arg(textFromValue(m_iOffsetEnd)), this);
					break;
				}
				x0 = pixelFromFrames(m_iOffsetStart);
				if (qAbs(x0 - x) < dx) {
					m_dragCursor = DragOffsetStart;
					QFrame::setCursor(QCursor(Qt::SizeHorCursor));
					QToolTip::showText(
						QCursor::pos(),
						tr("Offset start: %1")
							.arg(textFromValue(m_iOffsetStart)), this);
					break;
				}
			}
			if (m_dragCursor != DragNone) {
				m_dragCursor  = DragNone;
				QFrame::unsetCursor();
			}
		}
		break;
	case DragOffsetStart:
		if (m_pSample) {
			m_iDragOffsetStartX = safeX(x);
			if (m_iDragOffsetStartX > m_iDragOffsetEndX)
				m_iDragOffsetStartX = m_iDragOffsetEndX;
			if (m_bLoop && m_iDragOffsetStartX > m_iDragLoopStartX)
				m_iDragOffsetStartX = m_iDragLoopStartX;
			update();
			const uint32_t iOffsetStart
				= framesFromPixel(m_iDragOffsetStartX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset start: %1")
					.arg(textFromValue(iOffsetStart)), this);
		}
		break;
	case DragOffsetEnd:
		if (m_pSample) {
			m_iDragOffsetEndX = safeX(x);
			if (m_iDragOffsetEndX < m_iDragOffsetStartX)
				m_iDragOffsetEndX = m_iDragOffsetStartX;
			if (m_bLoop && m_iDragOffsetEndX < m_iDragLoopEndX)
				m_iDragOffsetEndX = m_iDragLoopEndX;
			update();
			const uint32_t iOffsetEnd
				= framesFromPixel(m_iDragOffsetEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset end: %1")
					.arg(textFromValue(iOffsetEnd)), this);
		}
		break;
	case DragOffsetRange:
		// Rubber-band offset selection...
		if (m_pSample) {
			const QRect& rect = QRect(m_posDrag, pMouseEvent->pos()).normalized();
			m_iDragOffsetStartX = safeX(rect.left());
			m_iDragOffsetEndX   = safeX(rect.right());
			if (m_bLoop) {
				if (m_iDragOffsetStartX > m_iDragLoopStartX)
					m_iDragOffsetStartX = m_iDragLoopStartX;
				if (m_iDragOffsetEndX < m_iDragLoopEndX)
					m_iDragOffsetEndX = m_iDragLoopEndX;
			}
			update();
			const uint32_t iOffsetStart
				= framesFromPixel(m_iDragOffsetStartX);
			const uint32_t iOffsetEnd
				= framesFromPixel(m_iDragOffsetEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Offset start: %1, end: %2")
					.arg(textFromValue(iOffsetStart))
					.arg(textFromValue(iOffsetEnd)), this);
		}
		break;
	case DragLoopStart:
		if (m_pSample) {
			m_iDragLoopStartX = safeX(x);
			if (m_bOffset && m_iDragLoopStartX < m_iDragOffsetStartX)
				m_iDragLoopStartX = m_iDragOffsetStartX;
			if (m_iDragLoopStartX > m_iDragLoopEndX)
				m_iDragLoopStartX = m_iDragLoopEndX;
			update();
			const uint32_t iLoopStart
				= framesFromPixel(m_iDragLoopStartX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Loop start: %1")
					.arg(textFromValue(iLoopStart)), this);
		}
		break;
	case DragLoopEnd:
		if (m_pSample) {
			m_iDragLoopEndX = safeX(x);
			if (m_bOffset && m_iDragLoopEndX > m_iDragOffsetEndX)
				m_iDragLoopEndX = m_iDragOffsetEndX;
			if (m_iDragLoopEndX < m_iDragLoopStartX)
				m_iDragLoopEndX = m_iDragLoopStartX;
			update();
			const uint32_t iLoopEnd
				= framesFromPixel(m_iDragLoopEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Loop end: %1")
					.arg(textFromValue(iLoopEnd)), this);
		}
		break;
	case DragLoopRange:
		// Rubber-band loop selection...
		if (m_pSample) {
			const QRect& rect = QRect(m_posDrag, pMouseEvent->pos()).normalized();
			m_iDragLoopStartX = safeX(rect.left());
			m_iDragLoopEndX   = safeX(rect.right());
			if (m_bOffset) {
				if (m_iDragLoopStartX < m_iDragOffsetStartX)
					m_iDragLoopStartX = m_iDragOffsetStartX;
				if (m_iDragLoopEndX > m_iDragOffsetEndX)
					m_iDragLoopEndX = m_iDragOffsetEndX;
				if (m_iDragLoopStartX > m_iDragLoopEndX)
					m_iDragLoopStartX = m_iDragLoopEndX;
			}
			update();
			const uint32_t iLoopStart
				= framesFromPixel(m_iDragLoopStartX);
			const uint32_t iLoopEnd
				= framesFromPixel(m_iDragLoopEndX);
			QToolTip::showText(
				QCursor::pos(),
				tr("Loop start: %1, end: %2")
					.arg(textFromValue(iLoopStart))
					.arg(textFromValue(iLoopEnd)), this);
		}
		break;
	case DragStart:
		// Rubber-band starting...
		if ((m_posDrag - pMouseEvent->pos()).manhattanLength()
			> QApplication::startDragDistance()) {
			// Start dragging alright...
			if (m_dragCursor != DragNone)
				m_dragState = m_dragCursor;
			else
			if (m_bOffset && pMouseEvent->modifiers() & Qt::ShiftModifier) {
				m_dragState = m_dragCursor = DragOffsetRange;
				m_iDragOffsetStartX = m_iDragOffsetEndX = m_posDrag.x();
				QFrame::setCursor(QCursor(Qt::SizeHorCursor));
			}
			else
			if (m_bLoop && (pMouseEvent->modifiers() & Qt::ControlModifier)) {
				m_dragState = m_dragCursor = DragLoopRange;
				m_iDragLoopStartX = m_iDragLoopEndX = m_posDrag.x();
				QFrame::setCursor(QCursor(Qt::SizeHorCursor));
			}
			else
			if (m_pSample && m_pSample->filename()) {
				QList<QUrl> urls;
				m_pDragSample = m_pSample;
				urls.append(QUrl::fromLocalFile(m_pDragSample->filename()));
				QMimeData *pMimeData = new QMimeData();
				pMimeData->setUrls(urls);;
				QDrag *pDrag = new QDrag(this);
				pDrag->setMimeData(pMimeData);
				pDrag->exec(Qt::CopyAction);
				resetDragState();
			}
		}
		// Fall thru...
	default:
		break;
	}

	QFrame::mouseMoveEvent(pMouseEvent);
}


void samplv1widget_sample::mouseReleaseEvent ( QMouseEvent *pMouseEvent )
{
	QFrame::mouseReleaseEvent(pMouseEvent);

	switch (m_dragState) {
	case DragOffsetStart:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetStart = framesFromPixel(m_iDragOffsetStartX);
			emit offsetRangeChanged();
		}
		break;
	case DragOffsetEnd:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetEnd = framesFromPixel(m_iDragOffsetEndX);
			emit offsetRangeChanged();
		}
		break;
	case DragOffsetRange:
		if (m_pSample && m_iDragOffsetStartX < m_iDragOffsetEndX) {
			m_iOffsetStart = framesFromPixel(m_iDragOffsetStartX);
			m_iOffsetEnd   = framesFromPixel(m_iDragOffsetEndX);
			emit offsetRangeChanged();
		}
		break;
	case DragLoopStart:
		if (m_pSample && m_iDragLoopStartX < m_iDragLoopEndX) {
			m_iLoopStart = framesFromPixel(m_iDragLoopStartX);
			emit loopRangeChanged();
		}
		break;
	case DragLoopEnd:
		if (m_pSample && m_iDragLoopStartX < m_iDragLoopEndX) {
			m_iLoopEnd = framesFromPixel(m_iDragLoopEndX);
			emit loopRangeChanged();
		}
		break;
	case DragLoopRange:
		if (m_pSample && m_iDragLoopStartX < m_iDragLoopEndX) {
			m_iLoopStart = framesFromPixel(m_iDragLoopStartX);
			m_iLoopEnd   = framesFromPixel(m_iDragLoopEndX);
			emit loopRangeChanged();
		}
		// Fall thru...
	default:
		break;
	}

	directNoteOff();

	m_pDragSample = nullptr;
	resetDragState();

	updateToolTip();
	update();
}


void samplv1widget_sample::mouseDoubleClickEvent ( QMouseEvent */*pMouseEvent*/ )
{
	openSample();
}


// Trap for escape key.
void samplv1widget_sample::keyPressEvent ( QKeyEvent *pKeyEvent )
{
	switch (pKeyEvent->key()) {
	case Qt::Key_Escape:
		m_pDragSample = nullptr;
		resetDragState();
		update();
		break;
	default:
		QFrame::keyPressEvent(pKeyEvent);
		break;
	}
}


// Drag-n-drop (more of the later) support.
void samplv1widget_sample::dragEnterEvent ( QDragEnterEvent *pDragEnterEvent )
{
	QFrame::dragEnterEvent(pDragEnterEvent);

	if (m_pDragSample && m_pDragSample == sample())
		return;

	if (pDragEnterEvent->mimeData()->hasUrls())
		pDragEnterEvent->acceptProposedAction();
}


void samplv1widget_sample::dropEvent ( QDropEvent *pDropEvent )
{
	QFrame::dropEvent(pDropEvent);

	const QMimeData *pMimeData = pDropEvent->mimeData();
	if (pMimeData->hasUrls()) {
		const QString& sFilename
			= QListIterator<QUrl>(pMimeData->urls()).peekNext().toLocalFile();
		if (!sFilename.isEmpty())
			emit loadSampleFile(sFilename);
	}
}


// Reset drag/select state.
void samplv1widget_sample::resetDragState (void)
{
	if (m_dragCursor != DragNone)
		QFrame::unsetCursor();

	m_iDragOffsetStartX = m_iDragOffsetEndX = 0;
	m_iDragLoopStartX = m_iDragLoopEndX = 0;

	m_dragState = m_dragCursor = DragNone;
}


void samplv1widget_sample::setEnvTime ( float fEnvTime )
{
	m_fEnvTime = qBound(0.0f, fEnvTime, 1.0f);

	update();
}


void samplv1widget_sample::setDca1Envelope(
	float fAttack, float fDecay, float fSustain, float fRelease )
{
	m_fDca1Attack  = qBound(0.0f, fAttack,  1.0f);
	m_fDca1Decay   = qBound(0.0f, fDecay,   1.0f);
	m_fDca1Sustain = qBound(0.0f, fSustain, 1.0f);
	m_fDca1Release = qBound(0.0f, fRelease, 1.0f);

	update();
}


void samplv1widget_sample::drawDca1Envelope (
	QPainter& painter, const QRect& rect, const QColor& rgbDca1 )
{
	// Background visual guide: GEN envelope time + DCA
	// This is visual only and does not affect audio processing.

	// ENV time range...
	const int h = rect.height();

	const float fSampleRateMs
		= 0.001f * m_pSample->sampleRate();
	const uint32_t iSampleLength
		= m_pSample->length();
	const uint32_t iFrameStart
		= (m_bOffset ? qMin(m_iOffsetStart, iSampleLength) : 0);
	const uint32_t iFrameEnd
		= (m_bOffset ? qMin(m_iOffsetEnd, iSampleLength) : iSampleLength);
	const uint32_t iFrameLength
		= (iFrameEnd > iFrameStart ? iFrameEnd - iFrameStart : 0);

	// Cap the envelope range at the active sample range.
	const float fMaxFrames
		= (m_fEnvTime < 0.005f
		? float(iFrameLength >> 1)
		: (10000.0f * fSampleRateMs * m_fEnvTime));

	const uint32_t iAttackLength
		= ::lrintf(fMaxFrames * m_fDca1Attack * m_fDca1Attack);
	const uint32_t iDecayLength
		= ::lrintf(fMaxFrames * m_fDca1Decay * m_fDca1Decay);
	const uint32_t iReleaseLength
		= ::lrintf(fMaxFrames * m_fDca1Release * m_fDca1Release);

	const uint32_t iAttackFrameEnd
		= qMin(iFrameEnd, iFrameStart + iAttackLength);
	const uint32_t iDecayFrameEnd
		= qMin(iFrameEnd, iAttackFrameEnd + iDecayLength);
	const uint32_t iReleaseFrameEnd
		= qMin(iFrameEnd, iFrameStart + iReleaseLength);

	const int x_start   = pixelFromFrames(iFrameStart);
	const int x_attack  = pixelFromFrames(iAttackFrameEnd);
	const int x_decay   = pixelFromFrames(iDecayFrameEnd);
	const int x_release = pixelFromFrames(iReleaseFrameEnd);
	const int x_end     = pixelFromFrames(iFrameEnd);

	auto lighten_rgb = [](const QColor& rgb, int amount) {
		return QColor(
			qMin(220, rgb.red()   + amount),
			qMin(220, rgb.green() + amount),
			qMin(220, rgb.blue()  + amount),
			rgb.alpha()
		);
	};

	const bool bSustainOn = m_fDca1Sustain > 0.001f;
	const bool bReleaseOn = m_fDca1Release > 0.001f;

	const QColor rgbAttackStart = lighten_rgb(rgbDca1, 0);
	const QColor rgbAttackEnd   = lighten_rgb(rgbDca1, 25);

	const QColor rgbDecayStart = lighten_rgb(rgbDca1, 30);
	const QColor rgbDecayEnd   = bSustainOn
		? lighten_rgb(rgbDca1, int(30.0f * m_fDca1Sustain))
		: rgbAttackStart;

	const QColor rgbSustain = bSustainOn
		? lighten_rgb(
			rgbDca1,
			// A - 15% of rgbDelayEnd
			int(30.0f * m_fDca1Sustain * (1.0f - 15.0f / 100.0f))
			)
		: lighten_rgb(rgbDca1, 10);

	const QColor rgbReleaseStart = QColor(255, 255, 255, 0);
	const QColor rgbReleaseEnd   = rgbDca1;
	const QColor rgbReleaseGuide = QColor(255, 255, 255, 50);

	const QColor rgbSilenceZone = lighten_rgb(rgbDca1, 20);

	painter.save();

	QFont font = painter.font();
	font.setPointSize(font.pointSize() - 3);
	painter.setFont(font);

	const QColor& rgbText
		= lighten_rgb(rgbDca1, 100);

	int labelFrame = 0;
	// Attack phase
	if (x_attack > x_start) {
		const QRect rect(x_start, 0, x_attack - x_start - 1, h);
		QLinearGradient grad(rect.left(), 0, rect.right(), 0);
		grad.setColorAt(0.0, rgbAttackStart);
		grad.setColorAt(1.0, rgbAttackEnd);
		painter.fillRect(rect, grad);
		painter.setPen(rgbText);
		painter.drawText(rect.adjusted(+2, 0, 0, -2),
			Qt::AlignLeft | Qt::AlignBottom, tr("Attack"));
		if (x_attack > 100) {
			labelFrame = iFrameStart + iAttackLength;
			if (labelFrame > iFrameEnd && iSampleLength > iFrameEnd)
				labelFrame = iFrameEnd;
			painter.drawText(rect.adjusted(0, 0, -2, -2),
				Qt::AlignRight | Qt::AlignBottom, QString::number(labelFrame));
		}
	}

	// Decay phase
	if (x_decay > x_attack) {
		const QRect rect(x_attack, 0, x_decay - x_attack - 1, h);
		QLinearGradient grad(rect.left(), 0, rect.right(), 0);
		grad.setColorAt(0.0, rgbDecayStart);
		grad.setColorAt(1.0, rgbDecayEnd);
		painter.fillRect(rect, grad);
		painter.setPen(rgbText);
		painter.drawText(rect.adjusted(+2, 0, 0, -2),
			Qt::AlignLeft | Qt::AlignBottom, tr("Decay"));
		if (x_decay - x_attack > 100){
			labelFrame = iFrameStart + iDecayLength + iAttackLength;
			if (labelFrame > iFrameEnd && iSampleLength > iFrameEnd)
				labelFrame = iFrameEnd;
			painter.drawText(rect.adjusted(0, 0, -2, -2),
				Qt::AlignRight | Qt::AlignBottom, QString::number(labelFrame));
		}
	}

	// Decay Silence
	if (x_end > x_decay && !bSustainOn) {
		const QRect rect(x_decay, 0, x_end - x_decay, h);
		painter.fillRect(rect, rgbDca1);
		painter.fillRect(rect, QBrush(rgbSilenceZone, Qt::BDiagPattern));
	}

	// Sustain phase
	if (x_end > x_decay && bSustainOn) {
		const QRect rect(x_decay, 0, x_end - x_decay, h);
		painter.fillRect(rect, rgbSustain);
		painter.setPen(rgbText);
		painter.drawText(rect.adjusted(+2, 0, 0, -2),
			Qt::AlignLeft | Qt::AlignBottom, tr("Sustain"));
	}

	// Release
	if (x_release > x_start) {
		const QRect rect(x_start, h - 25, x_release - x_start -1, 12);
		QLinearGradient grad(rect.left(), 0, rect.right(), 0);
		grad.setColorAt(0.0, rgbReleaseStart);
		grad.setColorAt(1.0, rgbReleaseEnd);
		painter.fillRect(rect, grad);

		painter.setPen(rgbReleaseGuide);
		painter.drawLine(rect.right(), h / (m_iChannels * 2), rect.right(), rect.bottom() -14);
	//	painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

		painter.setPen(rgbText);
		painter.drawText(rect.adjusted(+2, 0, 0, -2),
			Qt::AlignLeft | Qt::AlignBottom, tr("Release"));
		if (x_release > 100) {
			labelFrame = iFrameStart + iReleaseLength;
			if ( labelFrame > iFrameEnd && iSampleLength > iFrameEnd)
				labelFrame = iFrameEnd;
			painter.drawText(rect.adjusted(0, 0, -2, 0),
				Qt::AlignRight | Qt::AlignVCenter, QString::number(labelFrame));
		}
	}

	painter.restore();
}


// Draw curve.
void samplv1widget_sample::paintEvent ( QPaintEvent *pPaintEvent )
{
	QPainter painter(this);

	const QRect& rect = QFrame::rect();
	const int h = rect.height();
	const int w = rect.width();

	const QPalette& pal = palette();
	const bool bDark = (pal.window().color().value() < 0x7f);
	const QColor& rgbLite = (isEnabled() ? Qt::yellow : pal.mid().color());
	const QColor& rgbDark = pal.window().color().darker();

	painter.fillRect(rect, rgbDark);

	if (m_pSample && m_ppPolyg) {
		const bool bEnabled = isEnabled();
		QColor rgbLite1(rgbLite);
		QColor rgbDrop1(Qt::black);
		QColor rgbDca1(rgbDark);
		rgbLite1.setAlpha(bDark ? 120 : 160);
		rgbDrop1.setAlpha(80);
		const int w2 = (w << 1);
		painter.setRenderHint(QPainter::Antialiasing, true);
		// DCA1 EG...
		drawDca1Envelope(painter, rect, rgbDca1);
		// Crop { zeroLine
		painter.save();
		QPen zeroLine(rgbDark.darker(105));
		// zeroPen.setStyle(Qt::SolidLine);
		painter.setPen(zeroLine);
		// painter.setBrush(Qt::NoBrush);
		const int h0 = h / m_iChannels;
		for (unsigned short k = 0; k < m_iChannels; ++k) {
			const int yZero = rect.top() + (h0 >> 1) + k * h0;
			painter.drawLine(rect.left(), yZero, rect.right(), yZero);
		}
		painter.restore();
		// }
		// Loop point lines...
		if (m_bLoop && bEnabled) {
			int x1 = 0, x2 = 0;
			if (m_dragState == DragLoopStart ||
				m_dragState == DragLoopEnd   ||
				m_dragState == DragLoopRange) {
				x1 = m_iDragLoopStartX;
				x2 = m_iDragLoopEndX;
			} else {
				x1 = pixelFromFrames(m_iLoopStart);
				x2 = pixelFromFrames(m_iLoopEnd);
			}
			QLinearGradient grad1(0, 0, w2, h);
			painter.setPen(rgbLite1.lighter());
			grad1.setColorAt(0.0f, rgbLite1.darker(160));
			grad1.setColorAt(0.5f, rgbDrop1);
			painter.fillRect(x1, 0, x2 - x1, h, grad1);
			painter.drawLine(x1, 8, x1, h);
			painter.drawLine(x2, 0, x2, h - 8);
			painter.setBrush(rgbLite1.darker(160));
			QPolygon polyg(3);
			polyg.putPoints(0, 3, x1 + 8, 0, x1, 8, x1, 0);
			painter.drawPolygon(polyg);
		//	polyg.putPoints(0, 3, x1 + 8, h, x1, h - 8, x1, h);
		//	painter.drawPolygon(polyg);
		//	polyg.putPoints(0, 3, x2 - 8, 0, x2, 8, x2, 0);
		//	painter.drawPolygon(polyg);
			polyg.putPoints(0, 3, x2 - 8, h, x2, h - 8, x2, h);
			painter.drawPolygon(polyg);
		}
		// Sample waveform...
		QLinearGradient grad(0, 0, w2, h);
		painter.setPen(rgbLite1.darker(160));
		grad.setColorAt(0.0f, rgbLite1);
		grad.setColorAt(1.0f, rgbDrop1);
		painter.setBrush(grad);
		for (unsigned short k = 0; k < m_iChannels; ++k)
			painter.drawPolygon(*m_ppPolyg[k]);
		// Offset line...
		if (m_bOffset && bEnabled) {
			int x1 = 0, x2 = 0;
			if (m_dragState == DragOffsetStart ||
				m_dragState == DragOffsetEnd   ||
				m_dragState == DragOffsetRange) {
				x1 = m_iDragOffsetStartX;
				x2 = m_iDragOffsetEndX;
			} else {
				x1 = pixelFromFrames(m_iOffsetStart);
				x2 = pixelFromFrames(m_iOffsetEnd);
			}
			QColor rgbOver(rgbDark.darker(220));
			rgbOver.setAlpha(120);
			painter.setPen(rgbLite1.darker(160));
			painter.setBrush(rgbLite1.darker());
			QPolygon polyg(3);
		//	polyg.putPoints(0, 3, x1 + 8, 0, x1, 8, x1, 0);
		//	painter.drawPolygon(polyg);
			polyg.putPoints(0, 3, x1 + 8, h, x1, h - 8, x1, h);
			painter.drawPolygon(polyg);
			painter.fillRect(0, 0, x1, h, rgbOver);
			painter.drawLine(x1, 0, x1, h - 8);
			painter.drawLine(x2, 8, x2, h);
			polyg.putPoints(0, 3, x2 - 8, 0, x2, 8, x2, 0);
			painter.drawPolygon(polyg);
		//	polyg.putPoints(0, 3, x2 - 8, h, x2, h - 8, x2, h);
		//	painter.drawPolygon(polyg);
			painter.fillRect(x2, 0, w, h, rgbOver);
		}
		painter.setRenderHint(QPainter::Antialiasing, false);
	} else {
		painter.setPen(pal.midlight().color());
		painter.drawText(rect, Qt::AlignCenter,
			tr("(double-click or drop to load new sample...)"));
	}

	QString sTitle = m_sName;
	if (m_pSample && m_pSample->filename()) {
		if (!sTitle.isEmpty()) {
			sTitle += ' ';
			sTitle += '-';
			sTitle += ' ';
		}
		sTitle += QFileInfo(
			QString::fromUtf8(m_pSample->filename())
		).completeBaseName();
	}
	if (!sTitle.isEmpty()) {
		painter.setPen(pal.midlight().color());
		painter.drawText(rect.adjusted(2, 0, -2, -0), Qt::AlignLeft, sTitle);
	}

	painter.end();

	QFrame::paintEvent(pPaintEvent);
}


// Browse for a new sample.
void samplv1widget_sample::openSample (void)
{
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig == nullptr)
		return;

	QString sFilename = pConfig->sSampleDir;
	if (m_pSample && m_pSample->filename())
		sFilename = QString::fromUtf8(m_pSample->filename());

	// Cache supported file-types stuff from libsndfile...
	static QStringList s_extensions;
	if (s_extensions.isEmpty()) {
		s_extensions.append("aiff");
		s_extensions.append("flac");
		s_extensions.append("oga");
		s_extensions.append("ogg");
		s_extensions.append("wav");
	}

	static QStringList s_filters;
	if (s_filters.isEmpty()) {
		const QString sExtMask("*.%1");
		const QString sFilterMask("%1 (%2)");
		QStringList exts;
		SF_FORMAT_INFO sffinfo;
		int iCount = 0;
		::sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &iCount, sizeof(int));
		for (int i = 0 ; i < iCount; ++i) {
			sffinfo.format = i;
			::sf_command(nullptr, SFC_GET_FORMAT_MAJOR, &sffinfo, sizeof(sffinfo));
			const QString sFilterName = QString(sffinfo.name)
				.replace('/', '-') // Replace some illegal characters.
				.remove('(').remove(')');
			const QString sExtension(sffinfo.extension);
			QString sExt = sExtMask.arg(sExtension);
			QString sExts = sExt;
			if (!exts.contains(sExt)
				&& s_extensions.contains(sExtension)) {
				exts.append(sExt);
			}
			if (sExtension.length() > 3) {
				sExt = sExtMask.arg(sExtension.left(3));
				sExts += ' ' + sExt;
				if (!exts.contains(sExt)
					&& s_extensions.contains(sExtension)) {
					exts.append(sExt);
				}
			}
			if (sExtension == "oga") {
				sExt = sExtMask.arg("ogg");
				if (!exts.contains(sExt)) {
					sExts += ' ' + sExt;
					exts.append(sExt);
				}
			}
			s_filters.append(sFilterMask.arg(sFilterName).arg(sExts));
		}
		s_filters.prepend(sFilterMask.arg(tr("Audio files")).arg(exts.join(" ")));
		s_filters.append(sFilterMask.arg(tr("All files")).arg("*.*"));
	}

	const QString& sTitle  = tr("Open Sample");
	const QString& sFilter = s_filters.join(";;");
	QWidget *pParentWidget = nullptr;
	QFileDialog::Options options;
	if (pConfig->bDontUseNativeDialogs) {
		options |= QFileDialog::DontUseNativeDialog;
		pParentWidget = QWidget::window();
	}
#if 1//QT_VERSION < QT_VERSION_CHECK(4, 4, 0)
	sFilename = QFileDialog::getOpenFileName(pParentWidget,
		sTitle, sFilename, sFilter, nullptr, options);
#else
	QFileDialog fileDialog(pParentWidget, sTitle, sFilename, sFilter);
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFile);
	QList<QUrl> urls(fileDialog.sidebarUrls());
	urls.append(QUrl::fromLocalFile(pConfig->sSampleDir));
	fileDialog.setSidebarUrls(urls);
	fileDialog.setOptions(options);
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


// Value/text format converter utilities.
uint32_t samplv1widget_sample::valueFromText ( const QString& text ) const
{
	samplv1widget_spinbox::Format format = samplv1widget_spinbox::Frames;
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig)
		format = samplv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
	const float srate = (m_pSample ? m_pSample->sampleRate() : 44100.0f);
	return samplv1widget_spinbox::valueFromText(text, format, srate);
}


QString samplv1widget_sample::textFromValue ( uint32_t value ) const
{
	samplv1widget_spinbox::Format format = samplv1widget_spinbox::Frames;
	samplv1_config *pConfig = samplv1_config::getInstance();
	if (pConfig)
		format = samplv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
	const float srate = (m_pSample ? m_pSample->sampleRate() : 44100.0f);
	return samplv1widget_spinbox::textFromValue(value, format, srate);
}


// Update tool-tip.
void samplv1widget_sample::updateToolTip (void)
{
	QString sToolTip;

	if (!m_sName.isEmpty())
		sToolTip += '[' + m_sName + ']';

	const char *pszSampleFile = (m_pSample ? m_pSample->filename() : 0);
	if (pszSampleFile) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		QString suffix;
		samplv1widget_spinbox::Format format = samplv1widget_spinbox::Frames;
		samplv1_config *pConfig = samplv1_config::getInstance();
		if (pConfig)
			format = samplv1widget_spinbox::Format(pConfig->iFrameTimeFormat);
		if (format == samplv1widget_spinbox::Frames)
			suffix = tr(" frames");
		sToolTip += tr("%1\n%2%3, %4 channels, %5 Hz")
			.arg(QFileInfo(pszSampleFile).completeBaseName())
			.arg(textFromValue(m_pSample->length())).arg(suffix)
			.arg(m_pSample->channels())
			.arg(m_pSample->rate());
	}

	if (m_bOffset && m_iOffsetStart < m_iOffsetEnd) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		sToolTip += tr("Offset start: %1, end: %2")
			.arg(textFromValue(m_iOffsetStart))
			.arg(textFromValue(m_iOffsetEnd));
	}

	if (m_bLoop && m_iLoopStart < m_iLoopEnd) {
		if (!sToolTip.isEmpty()) sToolTip += '\n';
		sToolTip += tr("Loop start: %1, end: %2")
			.arg(textFromValue(m_iLoopStart))
			.arg(textFromValue(m_iLoopEnd));
	}

	setToolTip(sToolTip);
}


// Default size hint.
QSize samplv1widget_sample::sizeHint (void) const
{
	return QSize(480, 80);
}


// Direct note-on/off methods.
void samplv1widget_sample::directNoteOn (void)
{
	if (m_pSamplUi == nullptr || m_pSample == nullptr)
		return;

	const int key = int(m_pSamplUi->paramValue(samplv1::GEN1_SAMPLE));
	const float v = m_pSamplUi->paramValue(samplv1::DEF1_VELOCITY);
	const int vel = int(79.375f * v + 47.625f) & 0x7f;
	m_pSamplUi->directNoteOn(key, vel); // note-on!
	m_iDirectNoteOn = key;

	const float srate_ms = 0.001f * m_pSample->sampleRate();
	const int timeout_ms = int(float(m_pSample->length()) / srate_ms);
	QTimer::singleShot(timeout_ms, this, SLOT(directNoteOff()));
}


void samplv1widget_sample::directNoteOff (void)
{
	if (m_pSamplUi == nullptr || m_iDirectNoteOn < 0)
		return;

	m_pSamplUi->directNoteOn(m_iDirectNoteOn, 0); // note-off!

	m_iDirectNoteOn = -1;
}


// end of samplv1widget_sample.cpp
