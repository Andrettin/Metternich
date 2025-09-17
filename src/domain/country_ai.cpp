#include "metternich.h"

#include "domain/country_ai.h"

#include "domain/country.h"
#include "domain/country_economy.h"
#include "domain/country_game_data.h"
#include "domain/country_government.h"
#include "domain/country_military.h"
#include "domain/country_technology.h"
#include "domain/idea_type.h"
#include "domain/journal_entry.h"
#include "domain/office.h"
#include "infrastructure/building_type.h"
#include "infrastructure/building_type_container.h"
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

	this->assign_trade_orders();
}

void country_ai::choose_current_research()
{
	assert_throw(this->get_game_data()->is_ai());

	const data_entry_map<technology_category, const technology *> research_choice_map = this->country->get_technology()->get_research_choice_map(false);

	if (research_choice_map.empty()) {
		return;
	}

	const technology *chosen_technology = this->get_research_choice(research_choice_map);

	if (chosen_technology != nullptr) {
		this->country->get_technology()->add_current_research(chosen_technology);
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
	for (const office *office : this->country->get_government()->get_available_offices()) {
		if (!office->is_appointable()) {
			continue;
		}

		if (this->country->get_government()->get_office_holder(office) != nullptr) {
			continue;
		}

		if (this->country->get_government()->get_appointed_office_holder(office) != nullptr) {
			continue;
		}

		const character *character = this->country->get_government()->get_best_office_holder(office, nullptr);
		if (character != nullptr) {
			this->country->get_government()->set_appointed_office_holder(office, character);
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
