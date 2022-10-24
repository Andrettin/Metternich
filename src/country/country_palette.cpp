#include "metternich.h"

#include "country/country_palette.h"

#include "database/defines.h"
#include "util/container_util.h"

namespace metternich {

void country_palette::check() const
{
	const country_palette *conversible_color = defines::get()->get_conversible_country_palette();
	if (this->get_colors().size() != conversible_color->get_colors().size()) {
		throw std::runtime_error("The \"" + this->get_identifier() + "\" player color has a different amount of shades (" + std::to_string(this->get_colors().size()) + ") than the amount of shades (" + std::to_string(conversible_color->get_colors().size()) + ") for the conversible player color (\"" + conversible_color->get_identifier() + "\").");
	}
}

QVariantList country_palette::get_colors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_colors());
}

}
