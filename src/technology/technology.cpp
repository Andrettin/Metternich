#include "metternich.h"

#include "technology/technology.h"

#include "util/assert_util.h"

namespace metternich {

void technology::check() const
{
	assert_throw(!this->get_icon_filepath().empty());
}

void technology::set_icon_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_icon_filepath()) {
		return;
	}

	this->icon_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
