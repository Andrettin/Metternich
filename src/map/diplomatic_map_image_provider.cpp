#include "metternich.h"

#include "map/diplomatic_map_image_provider.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

QImage diplomatic_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const std::string &identifier = id_list.at(0);
	const bool selected = id_list.size() >= 2 && id_list.at(1) == "selected";

	const QImage *image = nullptr;

	if (identifier == "ocean") {
		image = &map::get()->get_ocean_diplomatic_map_image();
	} else if (identifier == "minimap") {
		image = &map::get()->get_minimap_image();
	} else if (identifier == "province") {
		const std::string &province_identifier = id_list.at(1);
		const province *province = province::get(province_identifier);
		const province_game_data *province_game_data = province->get_game_data();
		const bool province_selected = id_list.size() >= 3 && id_list.at(2) == "selected";

		image = province_selected ? &province_game_data->get_selected_province_map_image() : &province_game_data->get_province_map_image();
	} else {
		const country *country = country::get(identifier);
		const country_game_data *country_game_data = country->get_game_data();

		image = selected ? &country_game_data->get_selected_diplomatic_map_image() : &country_game_data->get_diplomatic_map_image();
	}

	assert_throw(image != nullptr);
	assert_log(!image->isNull());

	if (size != nullptr) {
		*size = image->size();
	}

	return *image;
}

}
