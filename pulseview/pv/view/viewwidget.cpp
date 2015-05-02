/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2014 Joel Holdsworth <joel@airwebreathe.org.uk>
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

#include <QApplication>
#include <QMouseEvent>
#include <QTouchEvent>

#include "rowitem.hpp"
#include "view.hpp"
#include "viewwidget.hpp"

using std::any_of;
using std::shared_ptr;
using std::vector;

namespace pv {
namespace view {

ViewWidget::ViewWidget(View &parent) :
	QWidget(&parent),
	view_(parent),
	item_dragging_(false)
{
	setFocusPolicy(Qt::ClickFocus);
	setAttribute(Qt::WA_AcceptTouchEvents, true);
	setMouseTracking(true);
}

void ViewWidget::clear_selection()
{
	const auto items = this->items();
	for (auto &i : items)
		i->select(false);
	update();
}

void ViewWidget::item_hover(const shared_ptr<ViewItem> &item)
{
	(void)item;
}

void ViewWidget::item_clicked(const shared_ptr<ViewItem> &item)
{
	(void)item;
}

bool ViewWidget::accept_drag() const
{
	const vector< shared_ptr<TimeItem> > items(view_.time_items());

	const bool any_row_items_selected = any_of(view_.begin(), view_.end(),
		[](const shared_ptr<RowItem> &r) { return r->selected(); });

	const bool any_time_items_selected = any_of(items.begin(), items.end(),
		[](const shared_ptr<TimeItem> &i) { return i->selected(); });

	if (any_row_items_selected && !any_time_items_selected)
	{
		// Check all the drag items share a common owner
		RowItemOwner *item_owner = nullptr;
		for (shared_ptr<RowItem> r : view_)
			if (r->dragging()) {
				if (!item_owner)
					item_owner = r->owner();
				else if(item_owner != r->owner())
					return false;
			}

		return true;
	}
	else if (any_time_items_selected && !any_row_items_selected)
	{
		return true;
	}

	// A background drag is beginning
	return true;
}

bool ViewWidget::mouse_down() const
{
	return mouse_down_point_.x() != INT_MIN &&
		mouse_down_point_.y() != INT_MIN;
}

void ViewWidget::drag_items(const QPoint &delta)
{
	bool item_dragged = false;

	// Drag the row items
	RowItemOwner *item_owner = nullptr;
	for (std::shared_ptr<RowItem> r : view_)
		if (r->dragging()) {
			item_owner = r->owner();
			r->drag_by(delta);

			// Ensure the trace is selected
			r->select();
		}

	if (item_owner) {
		item_dragged = true;
		item_owner->restack_items();
		for (const auto &r : *item_owner)
			r->animate_to_layout_v_offset();
	}

	// Drag the time items
	const vector< shared_ptr<TimeItem> > items(view_.time_items());
	for (auto &i : items)
		if (i->dragging()) {
			i->drag_by(delta);
			item_dragged = true;
		}

	// Do the background drag
	if (!item_dragged)
		drag_by(delta);
}

void ViewWidget::drag()
{
}

void ViewWidget::drag_by(const QPoint &delta)
{
	(void)delta;
}

void ViewWidget::drag_release()
{
}

void ViewWidget::mouse_left_press_event(QMouseEvent *event)
{
	(void)event;

	const bool ctrl_pressed =
		QApplication::keyboardModifiers() & Qt::ControlModifier;

	// Clear selection if control is not pressed and this item is unselected
	if ((!mouse_down_item_ || !mouse_down_item_->selected()) &&
		!ctrl_pressed)
		clear_selection();

	// Set the signal selection state if the item has been clicked
	if (mouse_down_item_) {
		if (ctrl_pressed)
			mouse_down_item_->select(!mouse_down_item_->selected());
		else
			mouse_down_item_->select(true);
	}

	// Save the offsets of any signals which will be dragged
	bool item_dragged = false;
	const auto items = this->items();
	for (auto &i : items)
		if (i->selected()) {
			item_dragged = true;
			i->drag();
		}

	// Do the background drag
	if (!item_dragged)
		drag();

	selection_changed();
	update();
}

void ViewWidget::mouse_left_release_event(QMouseEvent *event)
{
	assert(event);

	auto items = this->items();
	const bool ctrl_pressed =
		QApplication::keyboardModifiers() & Qt::ControlModifier;

	// Unselect everything if control is not pressed
	const shared_ptr<ViewItem> mouse_over =
		get_mouse_over_item(event->pos());

	for (auto &i : items)
		i->drag_release();

	if (item_dragging_)
		view_.restack_all_row_items();
	else
	{
		if (!ctrl_pressed) {
			for (shared_ptr<ViewItem> i : items)
				if (mouse_down_item_ != i)
					i->select(false);

			if (mouse_down_item_)
				item_clicked(mouse_down_item_);
		}
	}

	item_dragging_ = false;
}

bool ViewWidget::touch_event(QTouchEvent *e)
{
	(void)e;
	return false;
}

bool ViewWidget::event(QEvent *event)
{
	switch (event->type()) {
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd:
		if (touch_event(static_cast<QTouchEvent *>(event)))
			return true;
		break;

	default:
		break;
	}

	return QWidget::event(event);
}

void ViewWidget::mousePressEvent(QMouseEvent *event)
{
	assert(event);

	mouse_down_point_ = event->pos();
	mouse_down_item_ = get_mouse_over_item(event->pos());

	if (event->button() & Qt::LeftButton)
		mouse_left_press_event(event);
}

void ViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
	assert(event);
	if (event->button() & Qt::LeftButton)
		mouse_left_release_event(event);

	mouse_down_point_ = QPoint(INT_MIN, INT_MIN);
	mouse_down_item_ = nullptr;
}

void ViewWidget::mouseMoveEvent(QMouseEvent *e)
{
	assert(e);
	mouse_point_ = e->pos();

	if (!e->buttons())
		item_hover(get_mouse_over_item(e->pos()));
	else if (e->buttons() & Qt::LeftButton)
	{
		if (!item_dragging_)
		{
			if ((e->pos() - mouse_down_point_).manhattanLength() <
				QApplication::startDragDistance())
				return;

			if (!accept_drag())
				return;

			item_dragging_ = true;
		}

		// Do the drag
		drag_items(e->pos() - mouse_down_point_);

		update();
	}
}

void ViewWidget::leaveEvent(QEvent*)
{
	mouse_point_ = QPoint(-1, -1);
	update();
}

} // namespace view
} // namespace pv
