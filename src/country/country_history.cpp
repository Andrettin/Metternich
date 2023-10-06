#include "metternich.h"

#include "country/country_history.h"

#include "country/consulate.h"
#include "country/country.h"
#include "country/country_tier.h"
#include "country/diplomacy_state.h"
#include "util/map_util.h"

namespace metternich {

country_history::country_history(const metternich::country *country)
	: country(country), tier(country_tier::none)
{
}

void country_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "diplomacy_state") {
		const metternich::country *other_country = nullptr;
		std::optional<diplomacy_state> state;

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "country") {
				other_country = country::get(value);
			} else if (key == "state") {
				state = enum_converter<diplomacy_state>::to_enum(value);
			} else {
				throw std::runtime_error("Invalid diplomacy state property: \"" + key + "\".");
			}
		});

		if (other_country == nullptr) {
			throw std::runtime_error("Diplomacy state has no country.");
		}

		if (!state.has_value()) {
			throw std::runtime_error("Diplomacy state has no state.");
		}

		const bool is_vassalage = is_vassalage_diplomacy_state(state.value());

		if (is_vassalage) {
			//a country can only have one overlord, so remove any other vassalage states
			archimedes::map::remove_value_if(this->diplomacy_states, [](const diplomacy_state state) {
				return is_vassalage_diplomacy_state(state);
			});
		}

		this->diplomacy_states[other_country] = state.value();
	} else if (tag == "consulate") {
		const metternich::country *other_country = nullptr;
		const consulate *consulate = nullptr;

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "country") {
				other_country = country::get(value);
			} else if (key == "consulate") {
				consulate = consulate::get(value);
			} else {
				throw std::runtime_error("Invalid consulate property: \"" + key + "\".");
			}
		});

		if (other_country == nullptr) {
			throw std::runtime_error("Consulate history has no country.");
		}

		this->consulates[other_country] = consulate;
	} else {
		data_entry_history::process_gsml_scope(scope);
	}
}

}
