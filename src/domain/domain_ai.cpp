#include "metternich.h"

#include "domain/domain_ai.h"

#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_government.h"
#include "domain/domain_military.h"
#include "domain/domain_technology.h"
#include "domain/journal_entry.h"
#include "domain/office.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/building_type_container.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "technology/technology.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

domain_ai::domain_ai(const metternich::domain *domain) : domain(domain)
{
}

domain_ai::~domain_ai()
{
}

void domain_ai::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();

	throw std::runtime_error(std::format("Invalid domain AI property: \"{}\".", key));
}

void domain_ai::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "building_desire_modifiers") {
		scope.for_each_property([this](const gsml_property &property) {
			this->building_desire_modifiers[building_type::get(property.get_key())] = std::stoi(property.get_value());
		});
	} else if (tag == "holding_building_desire_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const site *holding_site = site::get(child_scope.get_tag());

			child_scope.for_each_property([this, holding_site](const gsml_property &property) {
				this->holding_building_desire_modifiers[holding_site][building_type::get(property.get_key())] = std::stoi(property.get_value());
			});
		});
	} else {
		throw std::runtime_error(std::format("Invalid domain AI scope: \"{}\".", tag));
	}
}

gsml_data domain_ai::to_gsml_data() const
{
	gsml_data data("ai");

	if (!this->building_desire_modifiers.empty()) {
		gsml_data desire_modifiers_data("building_desire_modifiers");
		for (const auto &[building_type, desire_modifier] : this->building_desire_modifiers) {
			desire_modifiers_data.add_property(building_type->get_identifier(), std::to_string(desire_modifier));
		}
		data.add_child(std::move(desire_modifiers_data));
	}

	if (!this->holding_building_desire_modifiers.empty()) {
		gsml_data holding_desire_modifiers_data("holding_building_desire_modifiers");

		for (const auto &[holding_site, building_desire_modifiers] : this->holding_building_desire_modifiers) {
			gsml_data desire_modifiers_data(holding_site->get_identifier());
			for (const auto &[building_type, desire_modifier] : building_desire_modifiers) {
				desire_modifiers_data.add_property(building_type->get_identifier(), std::to_string(desire_modifier));
			}
			holding_desire_modifiers_data.add_child(std::move(desire_modifiers_data));
		}

		data.add_child(std::move(holding_desire_modifiers_data));
	}

	return data;
}

domain_game_data *domain_ai::get_game_data() const
{
	return this->domain->get_game_data();
}

QCoro::Task<void> domain_ai::do_turn()
{
	assert_throw(this->get_game_data()->is_ai());

	//FIXME: add AI for recruiting civilian units

	this->appoint_office_holders();

	for (const province *province : this->get_game_data()->get_provinces()) {
		province->get_game_data()->do_ai_turn();
	}

	for (const qunique_ptr<civilian_unit> &civilian_unit : this->get_game_data()->get_civilian_units()) {
		civilian_unit->do_ai_turn();
	}

	for (const qunique_ptr<military_unit> &military_unit : this->get_game_data()->get_military()->get_military_units()) {
		military_unit->do_ai_turn();
	}

	for (size_t i = 0; i < this->get_game_data()->get_civilian_units().size();) {
		civilian_unit *civilian_unit = this->get_game_data()->get_civilian_units().at(i).get();
		if (civilian_unit->is_busy()) {
			++i;
		} else {
			//if the civilian unit is idle, this means that nothing was found for it to do above; in that case, disband it
			co_await civilian_unit->disband(false);
		}
	}

	this->assign_trade_orders();
}

const technology *domain_ai::get_research_choice(const data_entry_map<technology_category, const technology *> &research_choice_map) const
{
	assert_throw(this->get_game_data()->is_ai());

	std::vector<const technology *> preferred_technologies;

	int best_desire = 0;
	for (const auto &[category, technology] : research_choice_map) {
		int desire = 100 / (technology->get_total_prerequisite_depth() + 1);

		for (const journal_entry *journal_entry : this->get_game_data()->get_active_journal_entries()) {
			if (vector::contains(journal_entry->get_researched_technologies(), technology)) {
				desire += journal_entry::ai_technology_desire_modifier;
			}
		}

		assert_throw(desire > 0);

		if (desire > best_desire) {
			preferred_technologies.clear();
			best_desire = desire;
		}

		if (desire >= best_desire) {
			preferred_technologies.push_back(technology);
		}
	}

	assert_throw(!preferred_technologies.empty());

	const technology *chosen_technology = vector::get_random(preferred_technologies);
	return chosen_technology;
}

void domain_ai::appoint_office_holders()
{
	for (const office *office : this->domain->get_government()->get_available_offices()) {
		if (!office->is_appointable()) {
			continue;
		}

		if (this->domain->get_government()->get_office_holder(office) != nullptr) {
			continue;
		}

		if (this->domain->get_government()->get_appointed_office_holder(office) != nullptr) {
			continue;
		}

		const character *character = this->domain->get_government()->get_best_office_holder(office);
		if (character != nullptr) {
			this->domain->get_government()->set_appointed_office_holder(office, character);
		}
	}
}

void domain_ai::assign_trade_orders()
{
	assert_throw(this->get_game_data()->is_ai());

	domain_economy *domain_economy = this->get_game_data()->get_economy();

	domain_economy->clear_bids();
	domain_economy->clear_offers();

	if (this->get_game_data()->is_under_anarchy()) {
		return;
	}

	for (const auto &[commodity, value] : domain_economy->get_stored_commodities()) {
		if (!domain_economy->can_trade_commodity(commodity)) {
			continue;
		}

		const int need = domain_economy->get_commodity_need(commodity);

		if (value > need) {
			domain_economy->set_offer(commodity, value.to_int64());
		}
	}

	for (const commodity *commodity : domain_economy->get_tradeable_commodities()) {
		domain_economy->set_default_min_commodity_storage(commodity);
		domain_economy->set_default_max_commodity_storage(commodity);
	}
}

}

#include "moc_domain_ai.cpp"
