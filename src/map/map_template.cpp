#include "metternich.h"

#include "map/map_template.h"

#include "map/map_projection.h"
#include "map/province.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/geoshape_util.h"
#include "util/path_util.h"
#include "util/vector_util.h"

namespace metternich {

void map_template::set_province_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_province_image_filepath()) {
		return;
	}

	this->province_image_filepath = database::get()->get_maps_path(this->get_module()) / filepath;
}

void map_template::write_province_image()
{
	using province_geodata_map_type = province_map<std::vector<std::unique_ptr<QGeoShape>>>;

	assert_throw(this->get_world() != nullptr);

	province_geodata_map_type province_geodata_map = this->get_world()->parse_provinces_geojson_folder();

	color_map<std::vector<std::unique_ptr<QGeoShape>>> geodata_map;

	for (auto &[province, geoshapes] : province_geodata_map) {
		const QColor color = province->get_color();
		if (!color.isValid()) {
			throw std::runtime_error("Province \"" + province->get_identifier() + "\" has no valid color.");
		}

		vector::merge(geodata_map[color], std::move(geoshapes));
	}

	assert_throw(this->map_projection != nullptr);

	this->map_projection->validate_area(this->get_georectangle(), this->get_size());

	QImage base_image;

	if (!this->get_province_image_filepath().empty()) {
		base_image = QImage(path::to_qstring(this->get_province_image_filepath()));
		assert_throw(!base_image.isNull());
	}

	std::filesystem::path output_filepath = this->get_province_image_filepath().filename();
	if (output_filepath.empty()) {
		output_filepath = "provinces.png";
	}

	geoshape::write_image(output_filepath, geodata_map, this->get_georectangle(), this->get_size(), this->map_projection, base_image);
}

}
