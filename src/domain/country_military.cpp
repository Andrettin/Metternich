#include "metternich.h"

#include "domain/country_military.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "domain/country_economy.h"
#include "domain/country_government.h"
#include "domain/country_technology.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "engine_interface.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/and_condition.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"

namespace metternich {

country_military::country_military(const metternich::domain *domain)
	: domain(domain)
{
}

country_military::~country_military()
{
}

domain_game_data *country_military::get_game_data() const
{
	return this->domain->get_game_data();
}

void country_military::do_military_unit_recruitment()
{
	try {
		if (this->get_game_data()->is_under_anarchy()) {
			return;
		}

		for (const province *province : this->get_game_data()->get_provinces()) {
			province->get_game_data()->do_military_unit_recruitment();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing military unit recruitment for country \"{}\".", this->domain->get_identifier())));
	}
}

QVariantList country_military::get_leaders_qvariant_list() const
{
	return container::to_qvariant_list(this->get_leaders());
}

void country_military::add_leader(const character *leader)
{
	this->leaders.push_back(leader);

	emit leaders_changed();
}

void country_military::remove_leader(const character *leader)
{
	assert_throw(leader->get_game_data()->get_country() == this->domain);

	std::erase(this->leaders, leader);

	emit leaders_changed();
}

void country_military::clear_leaders()
{
	const std::vector<const character *> leaders = this->get_leaders();
	for (const character *leader : leaders) {
		this->remove_leader(leader);
	}

	assert_throw(this->get_leaders().empty());

	emit leaders_changed();
}

void country_military::on_leader_died(const character *leader)
{
	if (this->domain == game::get()->get_player_country()) {
		const portrait *war_minister_portrait = this->domain->get_government()->get_war_minister_portrait();

		const std::string_view leader_type_name = leader->get_leader_type_name();

		engine_interface::get()->add_notification(std::format("{} Retired", leader_type_name), war_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, the {} {} has decided to retire.", string::lowered(leader_type_name), leader->get_full_name()));
	}

	leader->get_game_data()->get_military_unit()->disband(true);
}

bool country_military::create_military_unit(const military_unit_type *military_unit_type, const province *deployment_province, const phenotype *phenotype, const std::vector<const promotion *> &promotions)
{
	if (deployment_province == nullptr) {
		deployment_province = this->get_game_data()->get_capital_province();
	}

	assert_throw(deployment_province != nullptr);
	assert_throw(deployment_province->get_game_data()->is_on_map());

	const character *chosen_character = nullptr;

	if (military_unit_type->get_unit_class()->is_leader()) {
		std::vector<const metternich::character *> potential_characters;

		for (const metternich::character *character : character::get_all()) {
			if (character->get_military_unit_category() != military_unit_type->get_category()) {
				continue;
			}

			if (character->get_game_data()->get_country() != nullptr && character->get_game_data()->get_country() != this->domain) {
				continue;
			}

			if (character->get_game_data()->get_military_unit() != nullptr) {
				continue;
			}

			if (phenotype != nullptr && character->get_phenotype() != phenotype) {
				continue;
			}

			if (character->get_game_data()->is_dead()) {
				continue;
			}

			if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->domain, read_only_context(this->domain))) {
				continue;
			}

			potential_characters.push_back(character);
		}

		if (!potential_characters.empty()) {
			chosen_character = vector::get_random(potential_characters);
		}
	}

	qunique_ptr<military_unit> military_unit;

	if (chosen_character != nullptr) {
		military_unit = make_qunique<metternich::military_unit>(military_unit_type, this->domain, chosen_character);
	} else {
		if (phenotype == nullptr) {
			const std::vector<const metternich::phenotype *> weighted_phenotypes = this->get_game_data()->get_weighted_phenotypes();
			assert_throw(!weighted_phenotypes.empty());
			phenotype = vector::get_random(weighted_phenotypes);
		}
		assert_throw(phenotype != nullptr);

		military_unit = make_qunique<metternich::military_unit>(military_unit_type, this->domain, phenotype);
	}

	assert_throw(military_unit != nullptr);

	military_unit->set_province(deployment_province);

	for (const promotion *promotion : promotions) {
		military_unit->add_promotion(promotion);
	}

	this->add_military_unit(std::move(military_unit));

	if (game::get()->is_running() && chosen_character != nullptr) {
		emit leader_recruited(chosen_character);
	}

	return true;
}

void country_military::add_military_unit(qunique_ptr<military_unit> &&military_unit)
{
	if (military_unit->get_character() != nullptr) {
		this->add_leader(military_unit->get_character());
	}

	if (military_unit->get_character() != nullptr) {
		military_unit->get_character()->get_game_data()->set_country(this->domain);
	}

	this->get_game_data()->add_unit_name(military_unit->get_name());
	this->military_units.push_back(std::move(military_unit));

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void country_military::remove_military_unit(military_unit *military_unit)
{
	if (military_unit->get_character() != nullptr) {
		this->remove_leader(military_unit->get_character());
	}

	if (military_unit->get_character() != nullptr) {
		assert_throw(military_unit->get_character()->get_game_data()->get_country() == this->domain);
		military_unit->get_character()->get_game_data()->set_country(nullptr);
	}

	this->get_game_data()->remove_unit_name(military_unit->get_name());

	for (size_t i = 0; i < this->military_units.size(); ++i) {
		if (this->military_units[i].get() == military_unit) {
			this->military_units.erase(this->military_units.begin() + i);
			return;
		}
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

int country_military::get_military_unit_type_cost_modifier(const military_unit_type *military_unit_type) const
{
	if (military_unit_type->is_infantry()) {
		return this->get_infantry_cost_modifier();
	} else if (military_unit_type->is_cavalry()) {
		return this->get_cavalry_cost_modifier();
	} else if (military_unit_type->is_artillery()) {
		return this->get_artillery_cost_modifier();
	} else if (military_unit_type->is_ship()) {
		return this->get_warship_cost_modifier();
	} else if (military_unit_type->get_unit_class()->is_leader()) {
		return this->get_leader_cost_modifier();
	}

	return 0;
}

commodity_map<int> country_military::get_military_unit_type_commodity_costs(const military_unit_type *military_unit_type, const int quantity) const
{
	commodity_map<int> commodity_costs = military_unit_type->get_commodity_costs();

	for (auto &[commodity, cost_int] : commodity_costs) {
		assert_throw(commodity->is_storable());

		centesimal_int cost(cost_int);
		cost *= quantity;

		const int cost_modifier = this->get_military_unit_type_cost_modifier(military_unit_type);
		cost *= 100 + cost_modifier;
		cost /= 100;

		cost_int = cost.to_int();

		if (cost_modifier < 0 && cost.get_fractional_value() > 0) {
			cost_int += 1;
		}

		if (quantity > 0) {
			cost_int = std::max(cost_int, 1);
		}
	}

	return commodity_costs;
}

QVariantList country_military::get_military_unit_type_commodity_costs_qvariant_list(const military_unit_type *military_unit_type, const int quantity) const
{
	return archimedes::map::to_qvariant_list(this->get_military_unit_type_commodity_costs(military_unit_type, quantity));
}

const military_unit_type *country_military::get_best_military_unit_category_type(const military_unit_category category, const culture *culture) const
{
	const military_unit_type *best_type = nullptr;
	int best_score = -1;

	for (const military_unit_class *military_unit_class : military_unit_class::get_all()) {
		if (military_unit_class->get_category() != category) {
			continue;
		}

		const military_unit_type *type = culture->get_military_class_unit_type(military_unit_class);

		if (type == nullptr) {
			continue;
		}

		if (type->get_required_technology() != nullptr && !this->domain->get_technology()->has_technology(type->get_required_technology())) {
			continue;
		}

		bool upgrade_is_available = false;
		for (const military_unit_type *upgrade : type->get_upgrades()) {
			if (culture->get_military_class_unit_type(upgrade->get_unit_class()) != upgrade) {
				continue;
			}

			if (upgrade->get_required_technology() != nullptr && !this->domain->get_technology()->has_technology(upgrade->get_required_technology())) {
				continue;
			}

			upgrade_is_available = true;
			break;
		}

		if (upgrade_is_available) {
			continue;
		}

		const int score = type->get_score();

		if (score > best_score) {
			best_type = type;
		}
	}

	return best_type;
}

const military_unit_type *country_military::get_best_military_unit_category_type(const military_unit_category category) const
{
	return this->get_best_military_unit_category_type(category, this->domain->get_culture());
}

void country_military::add_army(qunique_ptr<army> &&army)
{
	this->armies.push_back(std::move(army));
}

void country_military::remove_army(army *army)
{
	for (size_t i = 0; i < this->armies.size(); ++i) {
		if (this->armies[i].get() == army) {
			this->armies.erase(this->armies.begin() + i);
			return;
		}
	}
}

void country_military::clear_armies()
{
	this->armies.clear();
}

void country_military::set_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_military_unit_type_stat_modifier(type, stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->military_unit_type_stat_modifiers[type].erase(stat);

		if (this->military_unit_type_stat_modifiers[type].empty()) {
			this->military_unit_type_stat_modifiers.erase(type);
		}
	} else {
		this->military_unit_type_stat_modifiers[type][stat] = value;
	}

	const centesimal_int difference = value - old_value;
	for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
		if (military_unit->get_type() != type) {
			continue;
		}

		military_unit->change_stat(stat, difference);
	}
}

void country_military::set_free_infantry_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_infantry_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_infantry_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_infantry_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_infantry()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

void country_military::set_free_cavalry_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_cavalry_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_cavalry_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_cavalry_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_cavalry()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

void country_military::set_free_artillery_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_artillery_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_artillery_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_artillery_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_artillery()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

void country_military::set_free_warship_promotion_count(const promotion *promotion, const int value)
{
	const int old_value = this->get_free_warship_promotion_count(promotion);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_warship_promotion_counts.erase(promotion);
	} else if (old_value == 0) {
		this->free_warship_promotion_counts[promotion] = value;

		for (const qunique_ptr<military_unit> &military_unit : this->military_units) {
			if (!military_unit->get_type()->is_ship()) {
				continue;
			}

			military_unit->check_free_promotions();
		}
	}
}

}
