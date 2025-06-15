#include "metternich.h"

#include "ui/interface_image_provider.h"

#include "database/database.h"
#include "database/preferences.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

#include "xbrz.h"

namespace metternich {

interface_image_provider::interface_image_provider()
{
	QObject::connect(preferences::get(), &preferences::scale_factor_changed, [this]() {
		this->clear_images();
	});
}

QCoro::Task<void> interface_image_provider::load_image(const std::string id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &interface_style = id_list.at(0);
	const std::string &interface_element = id_list.at(1);

	bool is_frame_image = false;

	if (interface_element == "tiled_background") {
		is_frame_image = true;
	}

	std::filesystem::path filepath = database::get()->get_graphics_path(nullptr) / "interface" / interface_style / (interface_element + ".png");

	assert_throw(!filepath.empty());
	assert_throw(std::filesystem::exists(filepath));

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	centesimal_int image_scale_factor(1);

	const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	if (!scale_suffix_result.first.empty()) {
		filepath = scale_suffix_result.first;
		image_scale_factor = scale_suffix_result.second;
	}

	QImage image(path::to_qstring(filepath));
	assert_throw(!image.isNull());

	if (is_frame_image) {
		static constexpr QSize frame_size(32, 32);

		if (image_scale_factor != scale_factor) {
			co_await QtConcurrent::run([this, &image, &scale_factor, &image_scale_factor]() {
				image = image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, frame_size * image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
					xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
				});
			});

			const QSize scaled_frame_size = frame_size * preferences::get()->get_scale_factor();

			//load the entire image, and cache all frames
			std::vector<QImage> frame_images = image::to_frames(image, scaled_frame_size);

			const std::string base_id = id.substr(0, id.find_last_of('/'));

			for (size_t i = 0; i < frame_images.size(); ++i) {
				const std::string frame_id = base_id + "/" + std::to_string(i);
				this->set_image(frame_id, std::move(frame_images.at(i)));
			}
		}
	} else {
		if (image_scale_factor != scale_factor) {
			co_await QtConcurrent::run([this, &image, &scale_factor, &image_scale_factor]() {
				image = image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
					xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
				});
			});
		}

		this->set_image(id, std::move(image));
	}
}

}
