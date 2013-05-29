// samplv1widget_sample.h
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

#ifndef __samplv1widget_sample_h
#define __samplv1widget_sample_h

#include <QFrame>

#include <stdint.h>


// Forward decl.
class samplv1_sample;


//----------------------------------------------------------------------------
// samplv1widget_sample -- Custom widget

class samplv1widget_sample : public QFrame
{
	Q_OBJECT

public:

	// Constructor.
	samplv1widget_sample(QWidget *pParent = 0, Qt::WindowFlags wflags = 0);
	// Destructor.
	~samplv1widget_sample();

	// Parameter accessors.
	void setSample(samplv1_sample *pSample);
	samplv1_sample *sample() const;

	void setSampleName(const QString& sName);
	const QString& sampleName() const;

	void setLoop(bool bLoop);
	bool isLoop() const;

	// Loop point getters.
	uint32_t loopStart() const;
	uint32_t loopEnd() const;

signals:

	// Load new sample file.
	void loadSampleFile(const QString&);

	// Loop range changed.
	void loopRangeChanged();

public slots:

	// Browse for a new sample.
	void openSample();

	// Effective sample slot.
	void loadSample(samplv1_sample *pSample);

	// Loop point setters.
	void setLoopStart(uint32_t iLoopStart);
	void setLoopEnd(uint32_t iLoopEnd);

protected:

	// Widget resize handler.
	void resizeEvent(QResizeEvent *);

	// Draw canvas.
	void paintEvent(QPaintEvent *);

	// Mouse interaction.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);
	void mouseReleaseEvent(QMouseEvent *pMouseEvent);

	void mouseDoubleClickEvent(QMouseEvent *pMouseEvent);

	// Trap for escape key.
	void keyPressEvent(QKeyEvent *pKeyEvent);

	// Sanitizer helper;
	int safeX(int x) const;

	// Reset drag/select state.
	void resetDragState();

	// Update tool-tip.
	void updateToolTip();

private:

	// Instance state.
	samplv1_sample *m_pSample;
	unsigned short m_iChannels;
	QPolygon **m_ppPolyg;

	QString m_sName;

	// Drag state.
	enum DragState {
		DragNone = 0, DragStart, DragSelect,
		DragLoopStart, DragLoopEnd,
	} m_dragState, m_dragCursor;

	QPoint m_posDrag;

	int m_iDragStartX;
	int m_iDragEndX;

	// Loop state.
	bool     m_bLoop;
	uint32_t m_iLoopStart;
	uint32_t m_iLoopEnd;
};

#endif	// __samplv1widget_sample_h


// end of samplv1widget_sample.h
