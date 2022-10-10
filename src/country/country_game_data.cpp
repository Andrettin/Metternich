#include "metternich.h"

#include "country/country_game_data.h"

#include "country/country.h"
#include "country/diplomacy_state.h"

namespace metternich {

void country_game_data::add_province(const province *province)
{
	this->provinces.push_back(province);
}

void country_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);
}

diplomacy_state country_game_data::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

void country_game_data::set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state)
{
	if (is_vassalage_diplomacy_state(state)) {
		this->overlord = other_country;
	} else {
		if (this->overlord == other_country) {
			this->overlord = nullptr;
		}
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
	}
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

}
