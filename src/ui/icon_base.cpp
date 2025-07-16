#include "metternich.h"

#include "ui/icon_base.h"

#include "database/database.h"
#include "ui/icon_image_provider.h"
#include "util/assert_util.h"

namespace metternich {

void icon_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "hue_ignored_colors") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			this->hue_ignored_colors.insert(child_scope.to_color());
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

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
