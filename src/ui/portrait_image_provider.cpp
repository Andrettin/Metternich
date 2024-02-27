#include "metternich.h"

#include "ui/portrait_image_provider.h"

#include "database/preferences.h"
#include "ui/portrait.h"
#include "util/assert_util.h"
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
}

QCoro::Task<void> portrait_image_provider::load_image(const std::string id)
{
	const std::vector<std::string> id_list = string::split(id, '/');

	const std::string &identifier = id_list.at(0);
	const portrait *portrait = portrait::get(identifier);

	std::filesystem::path filepath = portrait->get_filepath();

	assert_throw(!filepath.empty());
	assert_throw(std::filesystem::exists(filepath));

	const centesimal_int scale_factor = preferences::get()->get_scale_factor();

	const std::pair<std::filesystem::path, centesimal_int> scale_suffix_result = image::get_scale_suffixed_filepath(filepath, scale_factor);

	centesimal_int image_scale_factor(1);

	if (!scale_suffix_result.first.empty()) {
		filepath = scale_suffix_result.first;
		image_scale_factor = scale_suffix_result.second;
	}

	QImage image(path::to_qstring(filepath));
	assert_throw(!image.isNull());

	if (image_scale_factor != scale_factor) {
		co_await QtConcurrent::run([this, &image, &scale_factor, &image_scale_factor]() {
			image = image::scale<QImage::Format_ARGB32>(image, scale_factor / image_scale_factor, [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
				xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
			});
		});
	}

	assert_throw(image.width() == (64 * scale_factor).to_int());
	assert_throw(image.height() == (64 * scale_factor).to_int());

	this->set_image(id, std::move(image));
}

}
