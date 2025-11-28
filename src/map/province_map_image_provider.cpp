#include "metternich.h"

#include "map/province_map_image_provider.h"

#include "domain/diplomacy_state.h"
#include "game/game.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_mode.h"
#include "util/assert_util.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

QImage province_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const std::string &identifier = id_list.at(0);

	const QImage *image = nullptr;

	if (identifier == "exploration") {
		image = &game::get()->get_exploration_diplomatic_map_image();
	} else {
		const province *province = province::get(identifier);
		const province_game_data *province_game_data = province->get_game_data();

		std::string_view mode_identifier;
		if (id_list.size() >= 2) {
			mode_identifier = id_list.at(1);
		}
		if (mode_identifier == "selected") {
			image = &province_game_data->get_selected_map_image();
		} else if (mode_identifier == "terrain") {
			image = &province_game_data->get_map_mode_image(province_map_mode::terrain);
		} else if (mode_identifier == "cultural") {
			image = &province_game_data->get_map_mode_image(province_map_mode::cultural);
		} else if (mode_identifier == "trade_zone") {
			image = &province_game_data->get_map_mode_image(province_map_mode::trade_zone);
		} else {
			image = &province_game_data->get_map_image();
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
