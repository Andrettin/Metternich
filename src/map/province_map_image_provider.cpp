#include "metternich.h"

#include "map/province_map_image_provider.h"

#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_mode.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

QImage province_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const std::string &identifier = id_list.at(0);

	QImage image;

	if (identifier == "exploration") {
		image = game::get()->get_exploration_diplomatic_map_image();
	} else {
		const province *province = province::get(identifier);
		const province_game_data *province_game_data = province->get_game_data();

		std::string_view mode_identifier;
		if (id_list.size() >= 2) {
			mode_identifier = id_list.at(1);
		}

		if (mode_identifier == "selected") {
			image = province_game_data->get_selected_map_image_promise()->future().result();
		} else if (mode_identifier == "interactive") {
			image = province_game_data->get_interactive_map_image_promise()->future().result();
		} else if (mode_identifier == "terrain") {
			image = province_game_data->get_map_mode_image_promise(province_map_mode::terrain)->future().result();
		} else if (mode_identifier == "cultural") {
			image = province_game_data->get_map_mode_image_promise(province_map_mode::cultural)->future().result();
		} else if (mode_identifier == "technology") {
			image = province_game_data->get_map_mode_image_promise(province_map_mode::technology)->future().result();
		} else if (mode_identifier == "trade_zone") {
			image = province_game_data->get_map_mode_image_promise(province_map_mode::trade_zone)->future().result();
		} else if (mode_identifier == "temple") {
			image = province_game_data->get_map_mode_image_promise(province_map_mode::temple)->future().result();
		} else {
			image = province_game_data->get_map_image_promise()->future().result();
		}
	}

	assert_log(!image.isNull());

	if (size != nullptr) {
		*size = image.size();
	}

	return image;
}

}
