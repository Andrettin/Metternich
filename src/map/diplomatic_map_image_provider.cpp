#include "metternich.h"

#include "map/diplomatic_map_image_provider.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/diplomacy_state.h"
#include "game/game.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

QImage diplomatic_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const std::string &identifier = id_list.at(0);

	const QImage *image = nullptr;

	if (identifier == "ocean") {
		image = &map::get()->get_ocean_diplomatic_map_image();
	} else if (identifier == "minimap") {
		image = &map::get()->get_minimap_image();
	} else if (identifier == "exploration") {
		image = &game::get()->get_exploration_diplomatic_map_image();
	} else {
		const country *country = country::get(identifier);
		const country_game_data *country_game_data = country->get_game_data();

		const std::string &mode_identifier = id_list.at(1);
		if (mode_identifier == "selected") {
			image = &country_game_data->get_selected_diplomatic_map_image();
		} else if (mode_identifier == "diplomatic") {
			std::optional<diplomacy_state> diplomacy_state;
			const std::string &diplomacy_state_identifier = id_list.at(2);
			if (diplomacy_state_identifier != "empire") {
				diplomacy_state = enum_converter<metternich::diplomacy_state>::to_enum(diplomacy_state_identifier);
			}
			
			if (diplomacy_state.has_value()) {
				image = &country_game_data->get_diplomacy_state_diplomatic_map_image(diplomacy_state.value());
			} else {
				image = &country_game_data->get_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic);
			}
		} else if (mode_identifier == "terrain") {
			image = &country_game_data->get_diplomatic_map_mode_image(diplomatic_map_mode::terrain);
		} else if (mode_identifier == "cultural") {
			image = &country_game_data->get_diplomatic_map_mode_image(diplomatic_map_mode::cultural);
		} else if (mode_identifier == "religious") {
			image = &country_game_data->get_diplomatic_map_mode_image(diplomatic_map_mode::religious);
		} else {
			image = &country_game_data->get_diplomatic_map_image();
		}
	}

	assert_throw(image != nullptr);
	assert_log(!image->isNull());

	if (size != nullptr) {
		*size = image->size();
	}

	return *image;
}

}
