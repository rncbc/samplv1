// samplv1widget_sample.h
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

#ifndef __samplv1widget_sample_h
#define __samplv1widget_sample_h

#include <QFrame>


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

signals:

	// Load new sample file.
	void loadSampleFile(const QString&);

public slots:

	// Browse for a new sample.
	void openSample();

	// Effective sample slot.
	void loadSample(samplv1_sample *pSample);

protected:

	// Widget resize handler.
	void resizeEvent(QResizeEvent *);

	// Mouse interaction.
	void mouseDoubleClickEvent(QMouseEvent *pMouseEvent);

	// Draw canvas.
	void paintEvent(QPaintEvent *);

private:

	// Instance state.
	samplv1_sample *m_pSample;
	unsigned short m_iChannels;
	QPolygon **m_ppPolyg;
};

#endif	// __samplv1widget_sample_h


// end of samplv1widget_sample.h
