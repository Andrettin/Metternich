#include "metternich.h"

#include "ui/icon_image_provider.h"

#include "database/preferences.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

#include "xbrz.h"

namespace metternich {

icon_image_provider::icon_image_provider()
{
	icon_image_provider::instance = this;

	QObject::connect(preferences::get(), &preferences::scale_factor_changed, [this]() {
		this->clear_images();
	});
}

QCoro::Task<void> icon_image_provider::load_image(const std::string id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &identifier = id_list.at(0);
	const icon *icon = icon::get(identifier);

	std::filesystem::path filepath = icon->get_filepath();

	assert_throw(!filepath.empty());
	assert_throw(std::filesystem::exists(filepath));

	centesimal_int scale_factor = preferences::get()->get_scale_factor();

	if (id_list.size() >= 2 && id_list.at(1) == "small") {
		scale_factor /= 2;
	}

	const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	centesimal_int image_scale_factor(1);

	if (!scale_suffix_result.first.empty()) {
		filepath = scale_suffix_result.first;
		image_scale_factor = scale_suffix_result.second;
	}

	QImage image(path::to_qstring(filepath));
	assert_throw(!image.isNull());

	if (id_list.size() >= 2) {
		const std::string &state = id_list.back();
		if (state == "selected") {
			static const QColor selected_color(Qt::white);
			image::set_outline_color(image, selected_color);
		} else if (state == "grayscale") {
			image::apply_grayscale(image);
		} else if (state == "green") {
			image::apply_greenscale(image);
		} else if (state == "red") {
			image::apply_redscale(image);
		} else if (state != "small") {
			assert_throw(false);
		}
	}

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
