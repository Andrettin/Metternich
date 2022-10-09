#include "metternich.h"

#include "database/defines.h"

#include "database/database.h"
#include "database/preferences.h"
#include "util/path_util.h"

namespace metternich {

defines::defines()
{
	connect(this, &defines::changed, this, &defines::scaled_tile_size_changed);
	connect(preferences::get(), &preferences::scale_factor_changed, this, &defines::scaled_tile_size_changed);
}

QSize defines::get_scaled_tile_size() const
{
	return this->get_tile_size() * preferences::get()->get_scale_factor();
}

int defines::get_scaled_tile_width() const
{
	return (this->get_tile_width() * preferences::get()->get_scale_factor()).to_int();
}

int defines::get_scaled_tile_height() const
{
	return (this->get_tile_height() * preferences::get()->get_scale_factor()).to_int();
}

void defines::set_default_settlement_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_default_settlement_image_filepath()) {
		return;
	}

	this->default_settlement_image_filepath = database::get()->get_graphics_filepath(filepath);
}

QString defines::get_default_menu_background_filepath_qstring() const
{
	return path::to_qstring(this->default_menu_background_filepath);
}

void defines::set_default_menu_background_filepath(const std::filesystem::path &filepath)
{
	this->default_menu_background_filepath = database::get()->get_graphics_filepath(filepath);
}

}
