#include "metternich.h"

#include "map/tile_image_provider.h"

#include "country/country_palette.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "infrastructure/improvement.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"
#include "util/thread_pool.h"

#include "xbrz.h"

namespace metternich {

tile_image_provider::tile_image_provider()
{
	tile_image_provider::instance = this;

	QObject::connect(preferences::get(), &preferences::scale_factor_changed, [this]() {
		this->clear_images();
	});
}

boost::asio::awaitable<void> tile_image_provider::load_image(const std::string &id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &tile_image_type = id_list.at(0);
	const std::string &identifier = id_list.at(1);
	std::filesystem::path filepath;

	bool is_frame_image = false;
	const country_palette *country_palette = nullptr;

	if (tile_image_type == "terrain") {
		const terrain_type *terrain = terrain_type::get(identifier);
		filepath = terrain->get_image_filepath();
		is_frame_image = true;
	} else if (tile_image_type == "settlement") {
		filepath = defines::get()->get_default_settlement_image_filepath();

		const std::string &palette_identifier = id_list.at(2);
		country_palette = country_palette::get(palette_identifier);
	} else if (tile_image_type == "improvement") {
		const improvement *improvement = improvement::get(identifier);
		filepath = improvement->get_image_filepath();
		is_frame_image = true;
	} else if (tile_image_type == "borders") {
		if (identifier == "province_border") {
			filepath = defines::get()->get_province_border_image_filepath();
		} else {
			assert_throw(false);
		}

		is_frame_image = true;
	} else {
		assert_throw(false);
	}

	assert_throw(!filepath.empty());
	assert_throw(std::filesystem::exists(filepath));

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();

	const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	centesimal_int image_scale_factor(1);

	if (!scale_suffix_result.first.empty()) {
		filepath = scale_suffix_result.first;
		image_scale_factor = scale_suffix_result.second;
	}

	QImage image(path::to_qstring(filepath));
	assert_throw(!image.isNull());

	if (country_palette != nullptr && country_palette != defines::get()->get_conversible_country_palette()) {
		country_palette->apply_to_image(image, defines::get()->get_conversible_country_palette());
	}

	if (image_scale_factor != scale_factor) {
		co_await thread_pool::get()->co_spawn_awaitable([this, &image, &scale_factor, &image_scale_factor]() -> boost::asio::awaitable<void> {
			image = co_await image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, defines::get()->get_tile_size() * image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
				xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
			});
		});
	}

	if (is_frame_image) {
		const QSize &frame_size = defines::get()->get_scaled_tile_size();

		//load the entire image, and cache all frames
		std::vector<QImage> frame_images = image::to_frames(image, frame_size);

		for (size_t i = 0; i < frame_images.size(); ++i) {
			const std::string frame_id = tile_image_type + "/" + identifier + "/" + std::to_string(i);
			this->set_image(frame_id, std::move(frame_images.at(i)));
		}
	} else {
		this->set_image(id, std::move(image));
	}
}

}
