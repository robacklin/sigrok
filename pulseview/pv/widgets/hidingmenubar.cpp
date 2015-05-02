/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2015 Joel Holdsworth <joel@airwebreathe.org.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <QKeyEvent>

#include "hidingmenubar.hpp"

namespace pv {
namespace widgets {

HidingMenuBar::HidingMenuBar(QWidget *parent) :
	QMenuBar(parent)
{
	setHidden(true);
	connect(this, SIGNAL(triggered(QAction*)),
		this, SLOT(item_triggered()));
}

void HidingMenuBar::focusOutEvent(QFocusEvent *e)
{
	if (e->reason() != Qt::PopupFocusReason)
		setHidden(true);
	QMenuBar::focusOutEvent(e);
}

void HidingMenuBar::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
		setHidden(true);
	QMenuBar::keyPressEvent(e);
}

void HidingMenuBar::item_triggered()
{
	setHidden(true);
}

}  // namespace widgets
}  // namespace pv
