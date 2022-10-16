#include "metternich.h"

#include "ui/icon.h"

#include "util/assert_util.h"

namespace metternich {

void icon::check() const
{
	assert_throw(!this->get_filepath().empty());
}

void icon::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
