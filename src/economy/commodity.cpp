#include "metternich.h"

#include "economy/commodity.h"

#include "util/assert_util.h"

namespace metternich {

void commodity::check() const
{
	assert_throw(!this->get_icon_filepath().empty());
}

void commodity::set_icon_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_icon_filepath()) {
		return;
	}

	this->icon_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
