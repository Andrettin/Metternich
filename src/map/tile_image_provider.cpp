#include "metternich.h"

#include "map/tile_image_provider.h"

#include "database/defines.h"
#include "map/terrain_type.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

namespace metternich {

void tile_image_provider::load_image(const std::string &id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &tile_image_type = id_list.at(0);
	const std::string &identifier = id_list.at(1);
	std::filesystem::path filepath;

	bool is_frame_image = false;

	if (tile_image_type == "terrain") {
		const terrain_type *terrain = terrain_type::get(identifier);
		filepath = terrain->get_image_filepath();
		is_frame_image = true;
	} else if (tile_image_type == "settlement") {
		filepath = defines::get()->get_default_settlement_image_filepath();
	} else {
		assert_throw(false);
	}

	assert_throw(!filepath.empty());

	QImage image(path::to_qstring(filepath));

	if (is_frame_image) {
		const QSize &frame_size = defines::get()->get_tile_size();

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
