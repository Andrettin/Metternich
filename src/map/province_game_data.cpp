#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"

namespace metternich {

void province_game_data::set_owner(const country *country)
{
	if (country == this->get_owner()) {
		return;
	}

	if (this->owner != nullptr) {
		this->owner->get_game_data()->remove_province(this->province);
	}

	this->owner = country;

	if (this->owner != nullptr) {
		this->owner->get_game_data()->add_province(this->province);
	}
}

}
