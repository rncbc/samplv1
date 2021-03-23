// samplv1widget_filt.h
//
/****************************************************************************
   Copyright (C) 2012-2021, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __samplv1widget_filt_h
#define __samplv1widget_filt_h

#include <QFrame>
#include <QPainterPath>


//----------------------------------------------------------------------------
// samplv1widget_filt -- Custom widget

class samplv1widget_filt : public QFrame
{
	Q_OBJECT

public:

	// Constructor.
	samplv1widget_filt(QWidget *pParent = nullptr);
	// Destructor.
	~samplv1widget_filt();

	// Parameter getters.
	float cutoff() const;
	float reso()   const;
	float type()   const;
	float slope()  const;

public slots:

	// Parameter setters.
	void setCutoff(float fCutoff);
	void setReso(float fReso);
	void setType(float fType);
	void setSlope(float fSlope);

signals:

	// Parameter change signals.
	void cutoffChanged(float fCutoff);
	void resoChanged(float fReso);

protected:

	// Draw canvas.
	void paintEvent(QPaintEvent *);

	// Filter types/slopes.
	enum Types {
		LPF = 0,
		BPF = 1,
		HPF = 2,
		BRF = 3,
		L2F = 4
	};

	enum Slopes {
		S12DB    = 0,
		S24DB    = 1,
		SBIQUAD  = 2,
		SFORMANT = 3
	};

	// Drag/move curve.
	void dragCurve(const QPoint& pos);

	// Mouse interaction.
	void mousePressEvent(QMouseEvent *pMouseEvent);
	void mouseMoveEvent(QMouseEvent *pMouseEvent);
	void mouseReleaseEvent(QMouseEvent *pMouseEvent);
	void wheelEvent(QWheelEvent *pWheelEvent);

	// Resize canvas.
	void resizeEvent(QResizeEvent *);

	// Update the drawing path.
	void updatePath();

private:

	// Instance state.
	float m_fCutoff;
	float m_fReso;
	int   m_iType;
	int   m_iSlope;

	// Drag state.
	bool m_bDragging;
	QPoint m_posDrag;

	// Drawable path.
	QPainterPath m_path;
};

#endif	// __samplv1widget_filt_h


// end of samplv1widget_filt.h
