#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country_palette final : public named_data_entry, public data_type<country_palette>
{
	Q_OBJECT

	Q_PROPERTY(QVariantList colors READ get_colors_qvariant_list)

public:
	static constexpr const char class_identifier[] = "country_palette";
	static constexpr const char property_class_identifier[] = "metternich::country_palette*";
	static constexpr const char database_folder[] = "country_palettes";

	explicit country_palette(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::vector<QColor> &get_colors() const
	{
		return this->colors;
	}

	QVariantList get_colors_qvariant_list() const;

	Q_INVOKABLE void add_color(const QColor &color)
	{
		this->colors.push_back(color);
	}

	void apply_to_image(QImage &image, const country_palette *conversible_palette) const
	{
		if (image.format() != QImage::Format_RGBA8888) {
			image = image.convertToFormat(QImage::Format_RGBA8888);
		}

		const int bpp = image.depth() / 8;

		if (bpp < 3) {
			throw std::runtime_error("Image BPP must be at least 3.");
		}

		unsigned char *image_data = image.bits();
		const std::vector<QColor> &conversible_colors = conversible_palette->get_colors();
		const std::vector<QColor> &colors = this->get_colors();

		for (int i = 0; i < image.sizeInBytes(); i += bpp) {
			unsigned char &red = image_data[i];
			unsigned char &green = image_data[i + 1];
			unsigned char &blue = image_data[i + 2];

			for (size_t z = 0; z < conversible_colors.size(); ++z) {
				const QColor &color = conversible_colors[z];
				if (red == color.red() && green == color.green() && blue == color.blue()) {
					red = colors[z].red();
					green = colors[z].green();
					blue = colors[z].blue();
				}
			}
		}
	}

private:
	std::vector<QColor> colors; //the color shades of the country color palette
};

}
