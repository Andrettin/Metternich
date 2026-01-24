#include "metternich.h"

#include "domain/domain_history.h"

#include "character/character.h"
#include "domain/consulate.h"
#include "domain/diplomacy_state.h"
#include "domain/domain.h"
#include "domain/domain_tier.h"
#include "domain/government_type.h"
#include "domain/law.h"
#include "domain/law_group.h"
#include "domain/office.h"
#include "domain/subject_type.h"
#include "economy/commodity.h"
#include "util/map_util.h"
#include "util/string_conversion_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

domain_history::domain_history(const metternich::domain *domain)
	: domain(domain), tier(domain_tier::none)
{
}

void domain_history::process_gsml_property(const gsml_property &property, const QDate &date)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "clear_regnal_numbering") {
		const bool clear_regnal_numbering = string::to_bool(value);
		if (clear_regnal_numbering) {
			this->historical_monarchs[date] = nullptr;
		}
	} else if (key == "titular_ruler") {
		//a titular ruler, who didn't actually govern, but is present in history
		const character *ruler = character::get(value);

		this->historical_rulers[date] = ruler;

		if (this->get_government_type() != nullptr && this->get_government_type()->has_regnal_numbering()) {
			this->historical_monarchs[date] = ruler;
		}
	} else {
		data_entry_history::process_gsml_property(property, date);
	}
}

void domain_history::process_gsml_scope(const gsml_data &scope, const QDate &date)
{
	const std::string &tag = scope.get_tag();

	if (tag == "offices") {
		scope.for_each_property([&](const gsml_property &property) {
			const office *office = office::get(property.get_key());
			const character *office_holder = character::get(property.get_value());
			if (office_holder != nullptr) {
				this->office_holders[office] = office_holder;

				if (office->is_ruler()) {
					this->historical_rulers[date] = office_holder;

					if (this->get_government_type() != nullptr && this->get_government_type()->has_regnal_numbering()) {
						this->historical_monarchs[date] = office_holder;
					}
				}
			} else {
				this->office_holders.erase(office);
			}
		});
	} else if (tag == "laws") {
		scope.for_each_property([&](const gsml_property &property) {
			const law_group *law_group = law_group::get(property.get_key());
			const law *law = law::get(property.get_value());
			if (law != nullptr) {
				this->laws[law_group] = law;
			} else {
				this->laws.erase(law_group);
			}
		});
	} else if (tag == "commodities") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodities[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "diplomacy_state") {
		const metternich::domain *other_domain = nullptr;
		std::optional<diplomacy_state> state;
		const metternich::subject_type *subject_type = nullptr;

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "country") {
				other_domain = domain::get(value);
			} else if (key == "state") {
				state = magic_enum::enum_cast<diplomacy_state>(value).value();
			} else if (key == "subject_type") {
				subject_type = subject_type::get(value);
				state = diplomacy_state::vassal;
			} else {
				throw std::runtime_error("Invalid diplomacy state property: \"" + key + "\".");
			}
		});

		if (other_domain == nullptr) {
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

			if (is_vassalage_diplomacy_state(this->get_diplomacy_state(other_domain))) {
				this->subject_type = nullptr;
			}
		}

		this->diplomacy_states[other_domain] = state.value();
	} else if (tag == "consulate") {
		const metternich::domain *other_domain = nullptr;
		const consulate *consulate = nullptr;

		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			if (key == "country") {
				other_domain = domain::get(value);
			} else if (key == "consulate") {
				consulate = consulate::get(value);
			} else {
				throw std::runtime_error("Invalid consulate property: \"" + key + "\".");
			}
		});

		if (other_domain == nullptr) {
			throw std::runtime_error("Consulate history has no country.");
		}

		this->consulates[other_domain] = consulate;
	} else {
		data_entry_history::process_gsml_scope(scope, date);
	}
}

diplomacy_state domain_history::get_diplomacy_state(const metternich::domain *other_domain) const
{
	const auto find_iterator = this->diplomacy_states.find(other_domain);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

}
