#include "metternich.h"

#include "database/defines.h"

#include "database/database.h"
#include "util/path_util.h"

namespace metternich {

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
