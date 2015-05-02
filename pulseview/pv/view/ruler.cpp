/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
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

#include <extdef.h>

#include <QApplication>
#include <QFontMetrics>
#include <QMouseEvent>

#include "ruler.hpp"
#include "view.hpp"

#include <pv/util.hpp>

using namespace Qt;

using std::shared_ptr;
using std::vector;

namespace pv {
namespace view {

const float Ruler::RulerHeight = 2.5f;  // x Text Height
const int Ruler::MinorTickSubdivision = 4;

const float Ruler::HoverArrowSize = 0.5f;  // x Text Height

Ruler::Ruler(View &parent) :
	MarginWidget(parent)
{
	setMouseTracking(true);

	connect(&view_, SIGNAL(hover_point_changed()),
		this, SLOT(hover_point_changed()));
}

QSize Ruler::sizeHint() const
{
	const int text_height = calculate_text_height();
	return QSize(0, RulerHeight * text_height);
}

QSize Ruler::extended_size_hint() const
{
	QRectF max_rect;
	std::vector< std::shared_ptr<TimeItem> > items(view_.time_items());
	for (auto &i : items)
		max_rect = max_rect.united(i->label_rect(QRect()));
	return QSize(0, sizeHint().height() - max_rect.top() / 2 +
		ViewItem::HighlightRadius);
}

vector< shared_ptr<ViewItem> > Ruler::items()
{
	const vector< shared_ptr<TimeItem> > time_items(view_.time_items());
	return vector< shared_ptr<ViewItem> >(
		time_items.begin(), time_items.end());
}

shared_ptr<ViewItem> Ruler::get_mouse_over_item(const QPoint &pt)
{
	const vector< shared_ptr<TimeItem> > items(view_.time_items());
	for (auto i = items.rbegin(); i != items.rend(); i++)
		if ((*i)->enabled() && (*i)->label_rect(rect()).contains(pt))
			return *i;
	return nullptr;
}

void Ruler::paintEvent(QPaintEvent*)
{
	const int ValueMargin = 3;

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const double tick_period = view_.tick_period();
	const unsigned int prefix = view_.tick_prefix();

	// Draw the tick marks
	p.setPen(palette().color(foregroundRole()));

	const double minor_tick_period = tick_period / MinorTickSubdivision;
	const double first_major_division =
		floor(view_.offset() / tick_period);
	const double first_minor_division =
		ceil(view_.offset() / minor_tick_period);
	const double t0 = first_major_division * tick_period;

	int division = (int)round(first_minor_division -
		first_major_division * MinorTickSubdivision) - 1;

	const int text_height = calculate_text_height();
	const int ruler_height = RulerHeight * text_height;
	const int major_tick_y1 = text_height + ValueMargin * 2;
	const int minor_tick_y1 = (major_tick_y1 + ruler_height) / 2;

	double x;

	do {
		const double t = t0 + division * minor_tick_period;
		x = (t - view_.offset()) / view_.scale();

		if (division % MinorTickSubdivision == 0)
		{
			// Draw a major tick
			p.drawText(x, ValueMargin, 0, text_height,
				AlignCenter | AlignTop | TextDontClip,
				pv::util::format_time(t, prefix));
			p.drawLine(QPointF(x, major_tick_y1),
				QPointF(x, ruler_height));
		}
		else
		{
			// Draw a minor tick
			p.drawLine(QPointF(x, minor_tick_y1),
				QPointF(x, ruler_height));
		}

		division++;

	} while (x < width());

	// Draw the hover mark
	draw_hover_mark(p, text_height);

	// The cursor labels are not drawn with the arrows exactly on the
	// bottom line of the widget, because then the selection shadow
	// would be clipped away.
	const QRect r = rect().adjusted(0, 0, 0, -ViewItem::HighlightRadius);

	// Draw the items
	const vector< shared_ptr<TimeItem> > items(view_.time_items());
	for (auto &i : items) {
		const bool highlight = !item_dragging_ &&
			i->label_rect(r).contains(mouse_point_);
		i->paint_label(p, r, highlight);
	}
}

void Ruler::mouseDoubleClickEvent(QMouseEvent *e)
{
	view_.add_flag(view_.offset() + ((double)e->x() + 0.5) * view_.scale());
}

void Ruler::draw_hover_mark(QPainter &p, int text_height)
{
	const int x = view_.hover_point().x();

	if (x == -1)
		return;

	p.setPen(QPen(Qt::NoPen));
	p.setBrush(QBrush(palette().color(foregroundRole())));

	const int b = RulerHeight * text_height;
	const float hover_arrow_size = HoverArrowSize * text_height;
	const QPointF points[] = {
		QPointF(x, b),
		QPointF(x - hover_arrow_size, b - hover_arrow_size),
		QPointF(x + hover_arrow_size, b - hover_arrow_size)
	};
	p.drawPolygon(points, countof(points));
}

int Ruler::calculate_text_height() const
{
	return QFontMetrics(font()).ascent();
}

void Ruler::hover_point_changed()
{
	update();
}

} // namespace view
} // namespace pv
