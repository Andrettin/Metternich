#include "metternich.h"

#include "country/country_history.h"

#include "country/consulate.h"
#include "country/country.h"
#include "country/country_tier.h"
#include "country/diplomacy_state.h"
#include "country/subject_type.h"
#include "economy/commodity.h"
#include "util/map_util.h"

namespace metternich {

country_history::country_history(const metternich::country *country)
	: country(country), tier(country_tier::none)
{
}

void country_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "commodities") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodities[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "diplomacy_state") {
		const metternich::country *other_country = nullptr;
		std::optional<diplomacy_state> state;
		const metternich::subject_type *subject_type = nullptr;

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "country") {
				other_country = country::get(value);
			} else if (key == "state") {
				state = enum_converter<diplomacy_state>::to_enum(value);
			} else if (key == "subject_type") {
				subject_type = subject_type::get(value);
				state = diplomacy_state::vassal;
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

			if (subject_type == nullptr) {
				throw std::runtime_error("Vassalage diplomacy state has no subject type.");
			}

			this->subject_type = subject_type;
		} else {
			if (subject_type != nullptr) {
				throw std::runtime_error("Non-vassalage diplomacy state has a subject type.");
			}

			if (is_vassalage_diplomacy_state(this->get_diplomacy_state(other_country))) {
				this->subject_type = nullptr;
			}
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

diplomacy_state country_history::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

}
