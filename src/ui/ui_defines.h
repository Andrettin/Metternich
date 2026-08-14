#pragma once

#include "database/ui_defines_base.h"
#include "util/singleton.h"

Q_MOC_INCLUDE("ui/cursor.h")

namespace metternich {

class cursor;

class ui_defines final : public ui_defines_base, public singleton<ui_defines>
{
	Q_OBJECT

	Q_PROPERTY(QColor magic_item_text_color MEMBER magic_item_text_color READ get_magic_item_text_color NOTIFY changed)
	Q_PROPERTY(metternich::cursor* default_cursor MEMBER default_cursor READ get_default_cursor NOTIFY changed)
	Q_PROPERTY(metternich::cursor* ally_target_cursor MEMBER ally_target_cursor READ get_ally_target_cursor NOTIFY changed)
	Q_PROPERTY(metternich::cursor* neutral_target_cursor MEMBER neutral_target_cursor READ get_neutral_target_cursor NOTIFY changed)
	Q_PROPERTY(metternich::cursor* enemy_target_cursor MEMBER enemy_target_cursor READ get_enemy_target_cursor NOTIFY changed)

public:
	using singleton<ui_defines>::get;

	ui_defines()
	{
		ui_defines_base::instance = this;
	}

	const QColor &get_magic_item_text_color() const
	{
		return this->magic_item_text_color;
	}

	cursor *get_default_cursor() const
	{
		return this->default_cursor;
	}

	cursor *get_ally_target_cursor() const
	{
		return this->ally_target_cursor;
	}

	cursor *get_neutral_target_cursor() const
	{
		return this->neutral_target_cursor;
	}

	cursor *get_enemy_target_cursor() const
	{
		return this->enemy_target_cursor;
	}

private:
	QColor magic_item_text_color;
	cursor *default_cursor = nullptr;
	cursor *ally_target_cursor = nullptr;
	cursor *neutral_target_cursor = nullptr;
	cursor *enemy_target_cursor = nullptr;
};

}
