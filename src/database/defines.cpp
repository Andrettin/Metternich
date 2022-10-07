#include "metternich.h"

#include "database/defines.h"

#include "database/database.h"
#include "util/path_util.h"

namespace metternich {

QString defines::get_default_menu_background_filepath_qstring() const
{
	return path::to_qstring(this->default_menu_background_filepath);
}

void defines::set_default_menu_background_filepath(const std::filesystem::path &filepath)
{
	this->default_menu_background_filepath = database::get()->get_graphics_filepath(filepath);
}

}
