#include "metternich.h"

#include "map/diplomatic_map_image_provider.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

QImage diplomatic_map_image_provider::requestImage(const QString &id, QSize *size, const QSize &requested_size)
{
	Q_UNUSED(id);
	Q_UNUSED(requested_size);

	const std::vector<std::string> id_list = string::split(id.toStdString(), '/');

	const std::string &country_identifier = id_list.at(0);
	const bool selected = id_list.size() >= 2 && id_list.at(1) == "selected";

	const country *country = country::get(country_identifier);
	const country_game_data *country_game_data = country->get_game_data();

	const QImage &image = selected ? country_game_data->get_selected_diplomatic_map_image() : country_game_data->get_diplomatic_map_image();

	assert_log(!image.isNull());

	if (size != nullptr) {
		*size = image.size();
	}

	return image;
}

}
