#include "metternich.h"

#include "ui/portrait_image_provider.h"

#include "database/preferences.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
#include "util/exception_util.h"
#include "util/image_util.h"
#include "util/path_util.h"
#include "util/string_util.h"

#include "xbrz.h"

namespace metternich {

portrait_image_provider::portrait_image_provider()
{
	portrait_image_provider::instance = this;

	QObject::connect(preferences::get(), &preferences::scale_factor_changed, [this]() {
		this->clear_images();
	});

	QObject::connect(preferences::get(), &preferences::scaling_algorithm_enabled_changed, [this]() {
		this->clear_images();
	});
}

QCoro::Task<void> portrait_image_provider::load_image(const std::string id)
{
	try {
		const std::vector<std::string> id_list = string::split(id, '/');

		const std::string &identifier = id_list.at(0);
		const portrait *portrait = portrait::get(identifier);

		std::filesystem::path filepath = portrait->get_filepath();

		assert_throw(!filepath.empty());
		assert_throw(std::filesystem::exists(filepath));

		centesimal_int scale_factor = preferences::get()->get_scale_factor();
		bool is_grayscale = false;

		if (id_list.size() >= 2) {
			const std::string &state = id_list.back();
			if (state == "grayscale") {
				is_grayscale = true;
			} else if (state == "small") {
				scale_factor /= 2;
			} else {
				assert_throw(false);
			}
		}

		centesimal_int image_scale_factor(1);

		if (preferences::get()->is_scaling_algorithm_enabled()) {
			const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

			if (!scale_suffix_result.first.empty()) {
				filepath = scale_suffix_result.first;
				image_scale_factor = scale_suffix_result.second;
			}
		}

		QImage image(path::to_qstring(filepath));
		assert_throw(!image.isNull());

		if (portrait->get_hue_rotation() != 0) {
			image::rotate_hue(image, portrait->get_hue_rotation(), portrait->get_hue_ignored_colors());
		}

		if (is_grayscale) {
			image::apply_grayscale(image);
		}

		if (image_scale_factor != scale_factor) {
			const bool scaling_algorithm_enabled = preferences::get()->is_scaling_algorithm_enabled();
			co_await QtConcurrent::run([this, &image, &scale_factor, &image_scale_factor, scaling_algorithm_enabled]() {
				if (scaling_algorithm_enabled) {
					image = image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
						xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
					});
				} else {
					image = image.scaled(image.size() * scale_factor);
				}
			});
		}

		const QSize expected_size((64 * scale_factor).to_int(), (64 * scale_factor).to_int());
		if (image.width() != expected_size.width()) {
			throw std::runtime_error(std::format("The portrait image for identifier \"{}\" has a width of {}, while {} was expected.", id, image.width(), expected_size.width()));
		}
		if (image.height() != expected_size.height()) {
			throw std::runtime_error(std::format("The portrait image for identifier \"{}\" has a height of {}, while {} was expected.", id, image.height(), expected_size.height()));
		}

		this->set_image(id, std::move(image));
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
	}
}

}
