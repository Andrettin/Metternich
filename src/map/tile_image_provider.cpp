#include "metternich.h"

#include "map/tile_image_provider.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "infrastructure/settlement_type.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

#include "xbrz.h"

namespace metternich {

tile_image_provider::tile_image_provider()
{
	tile_image_provider::instance = this;

	QObject::connect(preferences::get(), &preferences::scale_factor_changed, [this]() {
		this->clear_images();
	});
}

QCoro::Task<void> tile_image_provider::load_image(const std::string id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &tile_image_type = id_list.at(0);
	const std::string &identifier = id_list.at(1);
	std::filesystem::path filepath;

	bool is_subtile_image = false;
	bool is_frame_image = false;

	if (tile_image_type == "terrain") {
		const terrain_type *terrain = terrain_type::get(identifier);
		filepath = terrain->get_image_filepath();
		is_frame_image = true;

		if (!terrain->get_subtiles().empty()) {
			is_subtile_image = true;
		}
	} else if (tile_image_type == "settlement") {
		const settlement_type *settlement_type = settlement_type::get(identifier);
		filepath = settlement_type->get_image_filepath();
		is_frame_image = true;
	} else if (tile_image_type == "improvement") {
		const improvement *improvement = improvement::get(identifier);
		if (id_list.size() >= 4) {
			const std::string &terrain_identifier = id_list.at(2);
			const terrain_type *terrain = terrain_type::get(terrain_identifier);
			filepath = improvement->get_terrain_image_filepath(terrain);
		} else {
			filepath = improvement->get_image_filepath();
		}
		
		is_frame_image = true;
	} else if (tile_image_type == "pathway") {
		const pathway *pathway = pathway::get(identifier);
		filepath = pathway->get_image_filepath();
		
		is_frame_image = true;
	} else if (tile_image_type == "borders") {
		if (identifier == "province_border") {
			filepath = defines::get()->get_province_border_image_filepath();
		} else {
			assert_throw(false);
		}

		is_frame_image = true;
	} else if (tile_image_type == "river") {
		filepath = defines::get()->get_river_image_filepath();
		is_frame_image = true;
		is_subtile_image = true;

		if (id_list.at(1) == "-1") {
			//use a fully transparent image when the tile has no river
			QImage image(defines::get()->get_scaled_tile_size(), QImage::Format_ARGB32);
			image.fill(Qt::transparent);
			this->set_image(id, std::move(image));
			co_return;
		}
	} else if (tile_image_type == "rivermouth") {
		filepath = defines::get()->get_rivermouth_image_filepath();
		is_frame_image = true;
	} else {
		assert_throw(false);
	}

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	centesimal_int image_scale_factor(1);

	assert_throw(!filepath.empty());
	assert_throw(std::filesystem::exists(filepath));

	const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	if (!scale_suffix_result.first.empty()) {
		filepath = scale_suffix_result.first;
		image_scale_factor = scale_suffix_result.second;
	}

	QImage image = QImage(path::to_qstring(filepath));

	assert_throw(!image.isNull());

	const QSize frame_size = is_subtile_image ? defines::get()->get_tile_size() / 2 : defines::get()->get_tile_size();

	if (image_scale_factor != scale_factor) {
		co_await QtConcurrent::run([this, &image, &scale_factor, &image_scale_factor, frame_size]() {
			image = image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, frame_size * image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
				xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
			});
		});
	}

	if (is_frame_image) {
		const QSize scaled_frame_size = frame_size * preferences::get()->get_scale_factor();

		//load the entire image, and cache all frames
		std::vector<QImage> frame_images = image::to_frames(image, scaled_frame_size);

		const std::string base_id = id.substr(0, id.find_last_of('/'));

		for (size_t i = 0; i < frame_images.size(); ++i) {
			const std::string frame_id = base_id + "/" + std::to_string(i);
			this->set_image(frame_id, std::move(frame_images.at(i)));
		}
	} else {
		this->set_image(id, std::move(image));
	}
}

}
