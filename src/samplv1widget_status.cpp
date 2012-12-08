// samplv1widget_status.cpp
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

#include "samplv1widget_status.h"

#include "samplv1widget_config.h"

#include <QLabel>


//-------------------------------------------------------------------------
// samplv1widget_status - Custom status-bar widget.
//

// Constructor.
samplv1widget_status::samplv1widget_status ( QWidget *pParent )
	: QStatusBar (pParent)
{
}


// end of samplv1widget_status.cpp
