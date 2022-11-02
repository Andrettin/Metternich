#include "metternich.h"

#include "map/diplomatic_map_image_provider.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/preferences.h"
#include "game/game.h"
#include "map/map.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

QImage diplomatic_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const QImage *image = nullptr;
	QImage created_image;

	if (id_list.size() >= 1 && id_list.at(0) == "minimap") {
		image = &map::get()->get_minimap_image();
	} else if (id_list.size() >= 1 && id_list.at(0) == "country") {
		const std::string &identifier = id_list.at(1);
		const country *country = country::get(identifier);
		const country_game_data *country_game_data = country->get_game_data();
		created_image = country_game_data->get_diplomatic_map_image().scaled(country_game_data->get_diplomatic_map_image().size() * game::get()->get_diplomatic_map_scale_factor() * preferences::get()->get_scale_factor());
		image = &created_image;
	} else {
		const bool selected = id_list.size() >= 1 && id_list.at(0) == "selected";

		if (selected) {
			const std::string &identifier = id_list.at(1);
			const country *country = country::get(identifier);

			game::get()->set_diplomatic_map_selected_country(country);
		} else {
			game::get()->set_diplomatic_map_selected_country(nullptr);
		}

		image = &game::get()->get_scaled_diplomatic_map_image();
	}

	assert_throw(image != nullptr);
	assert_log(!image->isNull());

	if (size != nullptr) {
		*size = image->size();
	}

	return *image;
}

}
