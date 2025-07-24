#include "metternich.h"

#include "country/country_ai.h"

#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "country/country_military.h"
#include "country/idea_type.h"
#include "country/journal_entry.h"
#include "country/office.h"
#include "infrastructure/building_type.h"
#include "infrastructure/building_type_container.h"
#include "infrastructure/country_building_slot.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "technology/technology.h"
#include "unit/civilian_unit.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

country_ai::country_ai(const metternich::country *country) : country(country)
{
}

country_ai::~country_ai()
{
}

country_game_data *country_ai::get_game_data() const
{
	return this->country->get_game_data();
}

void country_ai::do_turn()
{
	assert_throw(this->get_game_data()->is_ai());

	//FIXME: add AI for recruiting civilian units

	this->choose_current_research();
	this->appoint_office_holders();
	this->appoint_ideas();

	//build buildings
	building_type_map<int> ai_building_desires;
	std::vector<const building_type *> ai_desired_buildings;
	for (const qunique_ptr<country_building_slot> &building_slot : this->get_game_data()->get_building_slots()) {
		if (!building_slot->is_available()) {
			continue;
		}

		const building_type *buildable_building = building_slot->get_buildable_building();

		if (buildable_building == nullptr) {
			continue;
		}

		if (building_slot->get_under_construction_building() != nullptr) {
			continue;
		}

		int ai_building_desire = 0;
		ai_building_desire += this->get_building_desire_modifier(buildable_building);

		if (ai_building_desire <= 0) {
			continue;
		}

		ai_building_desires[buildable_building] = ai_building_desire;
		ai_desired_buildings.push_back(buildable_building);
	}

	std::sort(ai_desired_buildings.begin(), ai_desired_buildings.end(), [&](const building_type *lhs, const building_type *rhs) {
		const int lhs_priority = ai_building_desires[lhs];
		const int rhs_priority = ai_building_desires[rhs];
		if (lhs_priority != rhs_priority) {
			return lhs_priority > rhs_priority;
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});

	for (const building_type *ai_desired_building : ai_desired_buildings) {
		country_building_slot *building_slot = this->get_game_data()->get_building_slot(ai_desired_building->get_slot_type());
		assert_throw(building_slot != nullptr);

		building_slot->build_building(ai_desired_building);
	}

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
			civilian_unit->disband(false);
		}
	}

	this->get_game_data()->get_economy()->assign_transport_orders();
	this->assign_trade_orders();
}

void country_ai::choose_current_research()
{
	assert_throw(this->get_game_data()->is_ai());

	const data_entry_map<technology_category, const technology *> research_choice_map = this->get_game_data()->get_research_choice_map(false);

	if (research_choice_map.empty()) {
		return;
	}

	const technology *chosen_technology = this->get_research_choice(research_choice_map);

	if (chosen_technology != nullptr) {
		this->get_game_data()->add_current_research(chosen_technology);
	}
}

const technology *country_ai::get_research_choice(const data_entry_map<technology_category, const technology *> &research_choice_map) const
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

void country_ai::appoint_ideas()
{
	magic_enum::enum_for_each<idea_type>([this](const idea_type idea_type) {
		for (const idea_slot *slot : this->get_game_data()->get_available_idea_slots(idea_type)) {
			if (this->get_game_data()->get_idea(slot) != nullptr) {
				continue;
			}

			if (this->get_game_data()->get_appointed_idea(slot) != nullptr) {
				continue;
			}

			const idea *idea = this->get_game_data()->get_best_idea(slot);
			if (idea != nullptr) {
				this->get_game_data()->set_appointed_idea(slot, idea);
			}
		}
	});
}

void country_ai::appoint_office_holders()
{
	for (const office *office : this->get_game_data()->get_available_offices()) {
		if (!office->is_appointable()) {
			continue;
		}

		if (this->get_game_data()->get_office_holder(office) != nullptr) {
			continue;
		}

		if (this->get_game_data()->get_appointed_office_holder(office) != nullptr) {
			continue;
		}

		const character *character = this->get_game_data()->get_best_office_holder(office, nullptr);
		if (character != nullptr) {
			this->get_game_data()->set_appointed_office_holder(office, character);
		}
	}
}

void country_ai::assign_trade_orders()
{
	assert_throw(this->get_game_data()->is_ai());

	this->get_game_data()->get_economy()->clear_bids();
	this->get_game_data()->get_economy()->clear_offers();

	if (this->get_game_data()->is_under_anarchy()) {
		return;
	}

	for (const auto &[commodity, value] : this->get_game_data()->get_economy()->get_stored_commodities()) {
		if (!this->get_game_data()->get_economy()->can_trade_commodity(commodity)) {
			continue;
		}

		const int need = this->get_game_data()->get_economy()->get_commodity_need(commodity);

		if (value > need) {
			this->get_game_data()->get_economy()->set_offer(commodity, value);
		}
	}
}

}
