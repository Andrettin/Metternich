#include "metternich.h"

#include "map/celestial_body_type.h"

#include "database/database.h"
#include "map/tile_image_provider.h"

namespace metternich {
	
celestial_body_type::celestial_body_type(const std::string &identifier) : named_data_entry(identifier)
{
}

celestial_body_type::~celestial_body_type()
{
}

void celestial_body_type::initialize()
{
	if (!this->get_image_filepath().empty()) {
		QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
			co_await tile_image_provider::get()->load_image("celestial_body/" + this->get_identifier() + "/0");
		});
	}

	named_data_entry::initialize();
}

void celestial_body_type::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

}
