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

	const std::string &terrain_identifier = id_list.at(0);
	const terrain_type *terrain = terrain_type::get(terrain_identifier);

	const std::filesystem::path &filepath = terrain->get_image_filepath();
	const QImage image(path::to_qstring(filepath));

	const QSize &frame_size = defines::get()->get_tile_size();

	//load the entire image, and cache all frames
	std::vector<QImage> frame_images = image::to_frames(image, frame_size);

	for (size_t i = 0; i < frame_images.size(); ++i) {
		const std::string frame_id = terrain_identifier + "/" + std::to_string(i);
		this->set_image(frame_id, std::move(frame_images.at(i)));
	}
}

}
