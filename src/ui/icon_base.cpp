#include "metternich.h"

#include "ui/icon_base.h"

#include "ui/icon_image_provider.h"
#include "util/assert_util.h"

namespace metternich {

void icon_base::check() const
{
	assert_throw(!this->get_filepath().empty());
}

void icon_base::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
