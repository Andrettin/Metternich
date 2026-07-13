#include "metternich.h"

#include "domain/domain_game_data.h"

#include "character/character.h"
#include "character/character_class.h"
#include "character/character_game_data.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/consulate.h"
#include "domain/country_type.h"
#include "domain/diplomacy_state.h"
#include "domain/domain.h"
#include "domain/domain_ai.h"
#include "domain/domain_attribute.h"
#include "domain/domain_economy.h"
#include "domain/domain_government.h"
#include "domain/domain_history.h"
#include "domain/domain_military.h"
#include "domain/domain_rank.h"
#include "domain/domain_technology.h"
#include "domain/domain_tier.h"
#include "domain/domain_tier_data.h"
#include "domain/domain_turn_data.h"
#include "domain/government_group.h"
#include "domain/government_type.h"
#include "domain/idea.h"
#include "domain/idea_slot.h"
#include "domain/idea_type.h"
#include "domain/journal_entry.h"
#include "domain/law.h"
#include "domain/subject_type.h"
#include "economy/commodity.h"
#include "economy/employment_type.h"
#include "economy/expense_transaction_type.h"
#include "economy/income_transaction_type.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_item_slot.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/holding_type.h"
#include "infrastructure/wonder.h"
#include "item/item.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_attribute.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/deity.h"
#include "religion/deity_slot.h"
#include "religion/religion.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/flag.h"
#include "script/modifier.h"
#include "script/opinion_modifier.h"
#include "script/scripted_domain_modifier.h"
#include "species/phenotype.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "unit/civilian_unit.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit.h"
#include "unit/military_unit_type.h"
#include "unit/transporter.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/date_util.h"
#include "util/gender.h"
#include "util/image_util.h"
#include "util/map_random_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/point_util.h"
#include "util/qunique_ptr.h"
#include "util/rect_util.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

domain_game_data::domain_game_data(metternich::domain *domain)
	: domain(domain), tier(domain_tier::none), culture(domain->get_default_culture()), religion(domain->get_default_religion())
{
	this->economy = make_qunique<domain_economy>(domain, this);
	this->government = make_qunique<domain_government>(domain, this);
	this->military = make_qunique<domain_military>(domain);
	this->technology = make_qunique<domain_technology>(domain, this);

	connect(this, &domain_game_data::tier_changed, this, &domain_game_data::title_name_changed);
	connect(this, &domain_game_data::government_type_changed, this, &domain_game_data::title_name_changed);
	connect(this, &domain_game_data::culture_changed, this, &domain_game_data::title_name_changed);
	connect(this, &domain_game_data::religion_changed, this, &domain_game_data::title_name_changed);
	connect(this, &domain_game_data::rank_changed, this, &domain_game_data::type_name_changed);

	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::population_unit_gained, this, &domain_game_data::on_population_unit_gained);

	connect(this, &domain_game_data::provinces_changed, this, &domain_game_data::income_changed);
	connect(this, &domain_game_data::provinces_changed, this, &domain_game_data::maintenance_cost_changed);
	connect(this, &domain_game_data::sites_changed, this, &domain_game_data::income_changed);
	connect(this, &domain_game_data::sites_changed, this, &domain_game_data::maintenance_cost_changed);
	connect(this->get_military(), &domain_military::military_units_changed, this, &domain_game_data::maintenance_cost_changed);
}

domain_game_data::~domain_game_data()
{
}

void domain_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "tier") {
		this->tier = magic_enum::enum_cast<domain_tier>(value).value();
	} else if (key == "culture") {
		this->culture = culture::get(value);
	} else if (key == "religion") {
		this->religion = religion::get(value);
	} else if (key == "government_type") {
		this->government_type = government_type::get(value);
	} else if (key == "capital") {
		this->capital = site::get(value);
	} else if (key == "holding_count") {
		this->holding_count = std::stoi(value);
	} else if (key == "consumption") {
		this->consumption = std::stoi(value);
	} else if (key == "unrest") {
		this->unrest = std::stoi(value);
	} else if (key == "domain_power") {
		this->domain_power = std::stoi(value);
	} else if (key == "max_current_constructions") {
		this->max_current_constructions = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid domain game data property: \"{}\".", key));
	}
}

void domain_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "attributes") {
		scope.for_each_property([this](const gsml_property &attribute_property) {
			this->attribute_values[domain_attribute::get(attribute_property.get_key())] = std::stoi(attribute_property.get_value());
		});
	} else if (tag == "site_attributes") {
		scope.for_each_property([this](const gsml_property &attribute_property) {
			this->site_attribute_values[site_attribute::get(attribute_property.get_key())] = std::stoi(attribute_property.get_value());
		});
	} else if (tag == "provinces") {
		for (const std::string &value : values) {
			this->provinces.push_back(province::get(value));
		}
	} else if (tag == "sites") {
		for (const std::string &value : values) {
			this->sites.push_back(site::get(value));
		}
	} else if (tag == "characters") {
		for (const std::string &value : values) {
			this->characters.push_back(game::get()->get_character(value));
		}
	} else if (tag == "historical_rulers") {
		scope.for_each_property([this](const gsml_property &attribute_property) {
			this->historical_rulers[string::to_date(attribute_property.get_key())] = game::get()->get_character(attribute_property.get_value());
		});
	} else if (tag == "historical_monarchs") {
		scope.for_each_property([this](const gsml_property &attribute_property) {
			this->historical_monarchs[string::to_date(attribute_property.get_key())] = game::get()->get_character(attribute_property.get_value());
		});
	} else if (tag == "civilian_units") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			auto civilian_unit = make_qunique<metternich::civilian_unit>(child_scope);
			this->add_civilian_unit(std::move(civilian_unit));
		});
	} else if (tag == "flags") {
		for (const std::string &value : values) {
			this->flags.insert(flag::get(value));
		}
	} else if (tag == "active_journal_entries") {
		for (const std::string &value : values) {
			this->active_journal_entries.push_back(journal_entry::get(value));
		}
	} else if (tag == "inactive_journal_entries") {
		for (const std::string &value : values) {
			this->inactive_journal_entries.push_back(journal_entry::get(value));
		}
	} else if (tag == "finished_journal_entries") {
		for (const std::string &value : values) {
			this->finished_journal_entries.push_back(journal_entry::get(value));
		}
	} else if (tag == "economy") {
		scope.process(this->get_economy());
	} else if (tag == "government") {
		scope.process(this->get_government());
	} else if (tag == "technology") {
		scope.process(this->get_technology());
	} else {
		throw std::runtime_error(std::format("Invalid domain game data scope: \"{}\".", tag));
	}
}

gsml_data domain_game_data::to_gsml_data() const
{
	gsml_data data(this->domain->get_identifier());

	data.add_property("tier", std::string(magic_enum::enum_name(this->get_tier())));
	data.add_property("culture", this->get_culture()->get_identifier());
	data.add_property("religion", this->get_religion()->get_identifier());

	if (this->get_government_type() != nullptr) {
		data.add_property("government_type", this->get_government_type()->get_identifier());
	}

	if (this->get_capital() != nullptr) {
		data.add_property("capital", this->get_capital()->get_identifier());
	}

	data.add_property("holding_count", std::to_string(this->get_holding_count()));
	data.add_property("consumption", std::to_string(this->get_consumption()));
	data.add_property("unrest", std::to_string(this->get_unrest()));
	data.add_property("domain_power", std::to_string(this->get_domain_power()));

	if (this->max_current_constructions != 0) {
		data.add_property("max_current_constructions", std::to_string(this->max_current_constructions));
	}

	if (!this->attribute_values.empty()) {
		gsml_data attributes_data("attributes");
		for (const auto &[attribute, value] : this->attribute_values) {
			attributes_data.add_property(attribute->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(attributes_data));
	}

	if (!this->site_attribute_values.empty()) {
		gsml_data attributes_data("site_attributes");
		for (const auto &[attribute, value] : this->site_attribute_values) {
			attributes_data.add_property(attribute->get_identifier(), std::to_string(value));
		}
		data.add_child(std::move(attributes_data));
	}

	gsml_data provinces_data("provinces");
	for (const province *province : this->get_provinces()) {
		provinces_data.add_value(province->get_identifier());
	}
	data.add_child(std::move(provinces_data));

	gsml_data sites_data("sites");
	for (const site *site : this->get_sites()) {
		sites_data.add_value(site->get_identifier());
	}
	data.add_child(std::move(sites_data));

	gsml_data characters_data("characters");
	for (const character *character : this->get_characters()) {
		characters_data.add_value(character->get_identifier());
	}
	data.add_child(std::move(characters_data));

	if (!this->historical_rulers.empty()) {
		gsml_data historical_rulers_data("historical_rulers");
		for (const auto &[date, historical_ruler] : this->historical_rulers) {
			historical_rulers_data.add_property(date::to_string(date), historical_ruler->get_identifier());
		}
		data.add_child(std::move(historical_rulers_data));
	}

	if (!this->historical_monarchs.empty()) {
		gsml_data historical_monarchs_data("historical_monarchs");
		for (const auto &[date, historical_monarch] : this->historical_monarchs) {
			historical_monarchs_data.add_property(date::to_string(date), historical_monarch->get_identifier());
		}
		data.add_child(std::move(historical_monarchs_data));
	}

	if (!this->get_civilian_units().empty()) {
		gsml_data civilian_units_data("civilian_units");
		for (const auto &civilian_unit : this->get_civilian_units()) {
			civilian_units_data.add_child(civilian_unit->to_gsml_data());
		}
		data.add_child(std::move(civilian_units_data));
	}

	if (!this->active_journal_entries.empty()) {
		gsml_data active_journal_entries_data("active_journal_entries");
		for (const journal_entry *journal_entry : this->active_journal_entries) {
			active_journal_entries_data.add_value(journal_entry->get_identifier());
		}
		data.add_child(std::move(active_journal_entries_data));
	}

	if (!this->inactive_journal_entries.empty()) {
		gsml_data inactive_journal_entries_data("inactive_journal_entries");
		for (const journal_entry *journal_entry : this->inactive_journal_entries) {
			inactive_journal_entries_data.add_value(journal_entry->get_identifier());
		}
		data.add_child(std::move(inactive_journal_entries_data));
	}

	if (!this->finished_journal_entries.empty()) {
		gsml_data finished_journal_entries_data("finished_journal_entries");
		for (const journal_entry *journal_entry : this->finished_journal_entries) {
			finished_journal_entries_data.add_value(journal_entry->get_identifier());
		}
		data.add_child(std::move(finished_journal_entries_data));
	}

	if (!this->flags.empty()) {
		gsml_data flags_data("flags");
		for (const flag *flag : this->flags) {
			flags_data.add_value(flag->get_identifier());
		}
		data.add_child(std::move(flags_data));
	}

	data.add_child(this->get_economy()->to_gsml_data());
	data.add_child(this->get_government()->to_gsml_data());
	data.add_child(this->get_technology()->to_gsml_data());

	return data;
}

QCoro::Task<void> domain_game_data::apply_history(const QDate &start_date)
{
	const domain_history *domain_history = this->domain->get_history();
	domain_economy *domain_economy = this->get_economy();
	domain_government *domain_government = this->get_government();
	domain_technology *domain_technology = this->get_technology();

	assert_throw(domain_history->get_owner() == nullptr);

	if (domain_history->get_tier() != domain_tier::none) {
		co_await this->set_tier(domain_history->get_tier());
	}

	if (domain_history->get_culture() != nullptr) {
		co_await this->set_culture(domain_history->get_culture());
	}

	if (domain_history->get_religion() != nullptr) {
		co_await this->set_religion(domain_history->get_religion());
	}

	const metternich::subject_type *subject_type = domain_history->get_subject_type();
	if (subject_type != nullptr) {
		co_await this->set_subject_type(subject_type);
	}

	if (domain_history->get_government_type() != nullptr) {
		co_await this->set_government_type(domain_history->get_government_type());

		if (domain_history->get_government_type()->get_required_technology() != nullptr) {
			co_await domain_technology->add_technology_with_prerequisites(domain_history->get_government_type()->get_required_technology());
		}
	} else if (this->domain->get_default_government_type() != nullptr) {
		co_await this->set_government_type(this->domain->get_default_government_type());

		if (this->domain->get_default_government_type()->get_required_technology() != nullptr) {
			co_await domain_technology->add_technology_with_prerequisites(this->domain->get_default_government_type()->get_required_technology());
		}
	}

	for (const auto &[office, office_holder] : domain_history->get_office_holders()) {
		assert_throw(start_date >= office_holder->get_game_data()->get_start_date());
		if (office_holder->get_game_data()->get_death_date().isValid() && start_date >= office_holder->get_game_data()->get_death_date()) {
			continue;
		}

		character_game_data *office_holder_game_data = office_holder->get_game_data();

		if (office_holder_game_data->get_domain() != nullptr && office_holder_game_data->get_domain() != this->domain) {
			throw std::runtime_error(std::format("Cannot set \"{}\" as an office holder for \"{}\", as they are already assigned to another domain.", office_holder->get_identifier(), this->domain->get_identifier()));
		}

		office_holder_game_data->set_domain(this->domain);
		co_await domain_government->set_office_holder(office, office_holder);
	}

	for (const metternich::technology *technology : domain_history->get_technologies()) {
		co_await domain_technology->add_technology_with_prerequisites(technology);
	}

	for (const auto &[law_group, law] : domain_history->get_laws()) {
		co_await domain_government->set_law(law_group, law);

		if (law->get_required_technology() != nullptr) {
			co_await domain_technology->add_technology_with_prerequisites(law->get_required_technology());
		}
	}

	domain_economy->set_wealth(domain_history->get_wealth());
}

void domain_game_data::apply_ruler_history(const QDate &start_date)
{
	const domain_history *domain_history = this->domain->get_history();

	this->historical_rulers = domain_history->get_historical_rulers();
	this->historical_monarchs = domain_history->get_historical_monarchs();
	for (const auto &[date, historical_ruler] : this->get_historical_rulers()) {
		historical_ruler->get_game_data()->add_ruled_domain(this->domain);
	}
	for (const auto &[date, historical_monarch] : this->get_historical_monarchs()) {
		historical_monarch->get_game_data()->add_reigned_domain(this->domain);
	}

	//add inserted domain ruler histories to the domain ruler history, but not to ruled domains of the characters
	for (const auto &[other_domain, insertion_date] : domain_history->get_inserted_domain_ruler_histories(start_date)) {
		const metternich::domain_history *other_domain_history = other_domain->get_history();
		for (const auto &[date, historical_ruler] : other_domain_history->get_historical_rulers()) {
			if (date > insertion_date) {
				break;
			}

			this->historical_rulers[date] = historical_ruler;
		}

		for (const auto &[date, historical_monarch] : other_domain_history->get_historical_monarchs()) {
			if (date > insertion_date) {
				break;
			}

			this->historical_monarchs[date] = historical_monarch;
		}
	}
}

QCoro::Task<void> domain_game_data::apply_diplomatic_history()
{
	const domain_history *domain_history = this->domain->get_history();

	for (const auto &[other_domain, diplomacy_state] : domain_history->get_diplomacy_states()) {
		if (!other_domain->get_game_data()->is_alive()) {
			continue;
		}

		co_await this->set_diplomacy_state(other_domain, diplomacy_state);
		co_await other_domain->get_game_data()->set_diplomacy_state(this->domain, get_diplomacy_state_counterpart(diplomacy_state));
	}

	for (const auto &[other_country, consulate] : domain_history->get_consulates()) {
		if (!other_country->get_game_data()->is_alive()) {
			continue;
		}

		this->set_consulate(other_country, consulate);
		other_country->get_game_data()->set_consulate(this->domain, consulate);
	}
}

QCoro::Task<void> domain_game_data::do_turn()
{
	try {
		for (const province *province : this->get_provinces()) {
			co_await province->get_game_data()->do_turn();
		}

		if (game::get()->is_last_turn_of_quarter()) {
			this->collect_regency();
			co_await this->get_economy()->do_production();
			this->collect_wealth();
			co_await this->pay_maintenance();
		}

		for (const character *character : this->get_characters()) {
			if (!character->get_game_data()->is_ruler()) {
				character->get_game_data()->ply_trade();
			}

			co_await character->get_game_data()->do_crafting();

			if (character->get_game_data()->is_ai()) {
				co_await character->get_game_data()->ai_sell_items();
				co_await character->get_game_data()->ai_buy_items();
			}

			const int64_t turn_days = game::get()->get_days_until_next_turn();
			context ctx(this->domain);
			co_await character->get_game_data()->decrement_status_effect_durations(std::chrono::days(turn_days), ctx);
		}

		if (game::get()->is_last_turn_of_quarter()) {
			//fill item slots after AI characters have bought items, so that the player has a chance to buy items before they are snatched up by AI characters
			this->check_item_slots();
		}

		this->do_transporter_recruitment();
		this->do_civilian_unit_recruitment();
		co_await this->get_military()->do_military_unit_recruitment();
		co_await this->get_technology()->do_research();
		co_await this->get_technology()->do_technology_spread();
		co_await this->do_construction();
		co_await this->do_population_growth();
		this->do_population_literacy_change();
		co_await this->do_population_cultural_change();
		co_await this->do_population_promotion();
		co_await this->do_population_employment();

		for (const qunique_ptr<civilian_unit> &civilian_unit : this->civilian_units) {
			co_await civilian_unit->do_turn();
		}

		for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
			co_await military_unit->do_turn();
		}

		for (const qunique_ptr<transporter> &transporter : this->transporters) {
			transporter->do_turn();
		}

		for (const qunique_ptr<army> &army : this->get_military()->get_armies()) {
			co_await army->do_turn();
		}

		this->get_military()->clear_armies();

		co_await this->decrement_scripted_modifiers();

		co_await this->check_journal_entries();

		co_await this->check_government_type();
		co_await this->check_characters();
		this->check_ideas();
		co_await this->check_tier();
		co_await this->check_culture();
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Failed to process turn for country \"{}\".", this->domain->get_identifier())));
	}
}

void domain_game_data::collect_regency()
{
	assert_throw(defines::get()->get_regency_commodity() != nullptr);

	if (this->get_government()->get_ruler() == nullptr) {
		return;
	}

	int collected_regency = 0;

	for (const province *province : this->get_provinces()) {
		collected_regency += province->get_game_data()->get_level();
	}

	data_entry_map<holding_type, int> total_holding_type_levels;
	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() || site->get_holding_type() == nullptr) {
			continue;
		}

		total_holding_type_levels[site->get_holding_type()] += site->get_game_data()->get_holding_level();
	}

	const character_class *ruler_character_class = this->get_government()->get_ruler()->get_game_data()->get_character_class();
	if (ruler_character_class != nullptr) {
		for (const auto &[holding_type, total_level] : total_holding_type_levels) {
			if (ruler_character_class->is_holding_type_favored(holding_type)) {
				collected_regency += total_level;
			} else if (ruler_character_class->is_holding_type_allowed(holding_type)) {
				collected_regency += total_level / 2;
			}
		}
	}

	if (collected_regency == 0) {
		return;
	}

	//limit the regency gain to the reputation score
	collected_regency = std::min(collected_regency, this->get_government()->get_ruler()->get_game_data()->get_reputation());

	this->get_economy()->add_tributable_commodity(defines::get()->get_regency_commodity(), collected_regency, income_transaction_type::tribute);
}

void domain_game_data::collect_wealth()
{
	//collect taxes from provinces
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->collect_taxes();
	}

	//collect income from holdings
	for (const site *holding_site : this->get_sites()) {
		if (!holding_site->is_settlement() || !holding_site->get_game_data()->is_built()) {
			continue;
		}

		holding_site->get_game_data()->collect_income();
	}

	//collect income from attributes
	for (const auto &[attribute, attribute_value] : this->get_attribute_values()) {
		if (!attribute->is_taxable()) {
			continue;
		}

		int result = 0;
		const bool success = this->do_attribute_check(attribute, 0, &result);
		if (!success) {
			continue;
		}

		result += attribute_value;
		result -= this->get_effective_unrest();
		result /= 3;

		if (result <= 0) {
			continue;
		}

		const commodity *wealth_commodity = defines::get()->get_wealth_commodity();
		const int64_t attribute_income = result * defines::get()->get_domain_income_unit_value();

		this->get_economy()->add_tributable_commodity(wealth_commodity, attribute_income, income_transaction_type::tribute);
		this->domain->get_turn_data()->add_income_transaction(income_transaction_type::income, attribute_income, nullptr, 0, this->domain);
	}
}

QCoro::Task<void> domain_game_data::pay_maintenance()
{
	const int64_t domain_maintenance_cost = std::min(this->get_domain_maintenance_cost(), this->get_economy()->get_stored_commodity(defines::get()->get_wealth_commodity()));
	if (domain_maintenance_cost != 0) {
		this->get_economy()->change_stored_commodity(defines::get()->get_wealth_commodity(), -domain_maintenance_cost);

		this->domain->get_turn_data()->add_expense_transaction(expense_transaction_type::domain_maintenance, domain_maintenance_cost, nullptr, 0, this->domain);
	}

	const int64_t domain_consumption_cost = std::min<int64_t>(std::max(this->get_consumption(), 0) * defines::get()->get_domain_income_unit_value(), this->get_economy()->get_stored_commodity(defines::get()->get_wealth_commodity()));
	if (domain_consumption_cost != 0) {
		this->get_economy()->change_stored_commodity(defines::get()->get_wealth_commodity(), -domain_consumption_cost);

		this->domain->get_turn_data()->add_expense_transaction(expense_transaction_type::consumption, domain_consumption_cost, nullptr, 0, this->domain);

		//FIXME: increase unrest if the domain cannot pay for its consumption
	}

	std::vector<military_unit *> military_units_to_disband;

	for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
		for (const auto &[commodity, maintenance_cost] : military_unit->get_type()->get_maintenance_commodity_costs()) {
			if (this->get_economy()->get_stored_commodity(commodity) >= maintenance_cost) {
				this->get_economy()->change_stored_commodity(commodity, -maintenance_cost);

				if (commodity == defines::get()->get_wealth_commodity()) {
					this->domain->get_turn_data()->add_expense_transaction(expense_transaction_type::military_maintenance, maintenance_cost, nullptr, 0, this->domain);
				} else {
					this->domain->get_turn_data()->add_expense_transaction(expense_transaction_type::military_maintenance, 0, commodity, maintenance_cost, this->domain);
				}
			} else {
				military_units_to_disband.push_back(military_unit.get());
			}
		}
	}

	for (military_unit *military_unit : military_units_to_disband) {
		this->domain->get_turn_data()->add_disbanded_military_unit(military_unit->get_type());
		co_await military_unit->disband(false);
	}
}

void domain_game_data::check_item_slots()
{
	for (const site *holding_site : this->get_sites()) {
		if (!holding_site->is_settlement() || !holding_site->get_game_data()->is_built()) {
			continue;
		}

		holding_site->get_game_data()->check_item_slots();
	}
}

void domain_game_data::do_civilian_unit_recruitment()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		const data_entry_map<civilian_unit_type, int> recruitment_counts = this->civilian_unit_recruitment_counts;
		for (const auto &[civilian_unit_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = this->create_civilian_unit(civilian_unit_type, nullptr, nullptr);
				const bool restore_costs = !created;
				this->change_civilian_unit_recruitment_count(civilian_unit_type, -1, restore_costs);
			}
		}

		assert_throw(this->civilian_unit_recruitment_counts.empty());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing civilian unit recruitment for country \"{}\".", this->domain->get_identifier())));
	}
}

void domain_game_data::do_transporter_recruitment()
{
	try {
		if (this->is_under_anarchy()) {
			return;
		}

		const transporter_type_map<int> recruitment_counts = this->transporter_recruitment_counts;
		for (const auto &[transporter_type, recruitment_count] : recruitment_counts) {
			assert_throw(recruitment_count > 0);

			for (int i = 0; i < recruitment_count; ++i) {
				const bool created = this->create_transporter(transporter_type, nullptr);
				const bool restore_costs = !created;
				this->change_transporter_recruitment_count(transporter_type, -1, restore_costs);
			}
		}

		assert_throw(this->transporter_recruitment_counts.empty());
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing transporter recruitment for country \"{}\".", this->domain->get_identifier())));
	}
}

QCoro::Task<void> domain_game_data::do_population_growth()
{
	try {
		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			co_return;
		}

		const int64_t available_food = this->get_available_food();

		int food_consumption = this->get_net_food_consumption();

		const int population_growth_change = static_cast<int>(available_food);
		this->change_population_growth(population_growth_change);

		if (population_growth_change > 0) {
			//food consumed for population growth
			food_consumption += population_growth_change;
		}
		this->do_food_consumption(food_consumption);

		while (this->get_population_growth() >= defines::get()->get_population_growth_threshold()) {
			co_await this->grow_population();
		}

		if (this->get_population_growth() < 0) {
			//co_await this->do_starvation();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error doing population growth for country \"{}\".", this->domain->get_identifier())));
	}
}

void domain_game_data::do_food_consumption(const int food_consumption)
{
	int64_t remaining_food_consumption = food_consumption;

	//this is a copy because we may need to erase elements from the map in the subsequent code
	const commodity_map<int64_t> stored_commodities = this->get_economy()->get_stored_commodities();

	for (const auto &[commodity, quantity] : stored_commodities) {
		if (commodity->is_food()) {
			const int64_t consumed_food = std::min(remaining_food_consumption, quantity);
			this->get_economy()->change_stored_commodity(commodity, -consumed_food);

			remaining_food_consumption -= consumed_food;

			if (remaining_food_consumption == 0) {
				break;
			}
		}
	}
}

QCoro::Task<void> domain_game_data::do_starvation()
{
	int starvation_count = 0;

	while (this->get_population_growth() < 0) {
		//starvation
		co_await this->decrease_population(true);
		++starvation_count;

		if (this->get_food_consumption() == 0) {
			this->set_population_growth(0);
			break;
		}
	}

	if (starvation_count > 0 && this->domain == game::get()->get_player_country()) {
		const bool plural = starvation_count > 1;

		const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

		engine_interface::get()->add_notification("Starvation", interior_minister_portrait, std::format("{}, I regret to inform you that {} {} of our population {} starved to death.", this->get_form_of_address(), number::to_formatted_string(starvation_count), (plural ? "units" : "unit"), (plural ? "have" : "has")));
	}
}

QCoro::Task<void> domain_game_data::do_construction()
{
	const commodity *construction_commodity = defines::get()->get_construction_commodity();

	const int64_t available_construction = this->get_economy()->get_stored_commodity(construction_commodity);

	this->get_economy()->set_stored_commodity(construction_commodity, 0);

	int under_construction_project_count = 0;
	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_under_construction_pathway() != nullptr) {
			++under_construction_project_count;
		}
	}
	for (const site *site : this->get_sites()) {
		if (site->is_settlement() && site->get_game_data()->is_built()) {
			for (const auto &building_slot : site->get_game_data()->get_building_slots()) {
				if (building_slot->get_under_construction_building() != nullptr) {
					if (building_slot->is_available()) {
						++under_construction_project_count;
					} else {
						building_slot->cancel_construction();
					}
				}
			}
		}
	}

	const int available_current_construction_slots = this->get_max_current_constructions() - under_construction_project_count;
	for (int i = 0; i < available_current_construction_slots; ++i) {
		const bool construction_chosen = co_await this->choose_construction();
		if (construction_chosen) {
			++under_construction_project_count;
		}
	}

	if (under_construction_project_count == 0) {
		co_return;
	}

	const decimillesimal_int construction_per_project = decimillesimal_int(available_construction) / under_construction_project_count;

	for (const province *province : this->get_provinces()) {
		co_await province->get_game_data()->do_construction(construction_per_project);
	}

	for (const site *site : this->get_sites()) {
		if (site->is_settlement() && site->get_game_data()->is_built()) {
			co_await site->get_game_data()->do_construction(construction_per_project);
		}
	}
}

void domain_game_data::do_population_literacy_change()
{
	for (const province *province : this->get_provinces()) {
		province->get_game_data()->do_population_literacy_change();
	}
}

QCoro::Task<void> domain_game_data::do_population_cultural_change()
{
	const std::vector<population_unit *> population_units = this->get_population_units();
	for (population_unit *population_unit : population_units) {
		co_await population_unit->do_cultural_change();
	}
}

QCoro::Task<void> domain_game_data::do_population_promotion()
{
	const std::vector<population_unit *> population_units = this->get_population_units();
	for (population_unit *population_unit : population_units) {
		co_await population_unit->do_promotion();
	}
}

QCoro::Task<void> domain_game_data::do_population_employment()
{
	for (const site *site : this->get_sites()) {
		if (site->is_settlement() && site->get_game_data()->is_built()) {
			co_await site->get_game_data()->check_employment();
		}
	}
}

QCoro::Task<void> domain_game_data::do_events()
{
	const std::vector<const province *> provinces = this->get_provinces();
	for (const province *province : provinces) {
		co_await province->get_game_data()->do_events();
	}

	const std::vector<const site *> sites = this->get_sites();
	for (const site *site : sites) {
		co_await site->get_game_data()->do_events();
	}

	if (!this->is_under_anarchy()) {
		const bool is_last_turn_of_year = game::get()->is_last_turn_of_year();
		if (is_last_turn_of_year) {
			co_await domain_event::check_events_for_scope(this->domain, event_trigger::yearly_pulse);
		}

		const bool is_last_turn_of_quarter = game::get()->is_last_turn_of_quarter();
		if (is_last_turn_of_quarter) {
			co_await domain_event::check_events_for_scope(this->domain, event_trigger::quarterly_pulse);
		}

		co_await domain_event::check_events_for_scope(this->domain, event_trigger::per_turn_pulse);
	}

	const std::vector<const character *> characters = this->get_characters();
	for (const character *character : characters) {
		co_await character->get_game_data()->do_events();
	}
}

bool domain_game_data::is_ai() const
{
	return this->domain != game::get()->get_player_country();
}

domain_ai *domain_game_data::get_ai() const
{
	return this->domain->get_ai();
}

QCoro::Task<void> domain_game_data::set_tier(const domain_tier tier)
{
	if (tier == this->get_tier()) {
		co_return;
	}

	assert_throw(tier >= this->domain->get_min_tier());
	assert_throw(tier <= this->domain->get_max_tier());

	const domain_tier old_tier = this->get_tier();

	if (this->get_tier() != domain_tier::none) {
		const domain_tier_data *tier_data = domain_tier_data::get(this->get_tier());
		if (tier_data->get_modifier() != nullptr) {
			co_await tier_data->get_modifier()->remove(this->domain);
		}
	}

	this->tier = tier;

	if (this->get_tier() != domain_tier::none) {
		const domain_tier_data *tier_data = domain_tier_data::get(this->get_tier());
		if (tier_data->get_modifier() != nullptr) {
			co_await tier_data->get_modifier()->apply(this->domain);
		}
	}

	for (const site *site : this->get_sites()) {
		site->get_game_data()->update_holding_type_name();
	}

	if (tier < old_tier) {
		//if the domain's tier decreases to the point that it is no longer above the tier of any of its vassals, that vassal then becomes independent
		const std::vector<const metternich::domain *> vassals = this->get_vassals();
		for (const metternich::domain *vassal : vassals) {
			if (this->get_tier() <= vassal->get_game_data()->get_tier()) {
				co_await this->set_diplomacy_state(vassal, diplomacy_state::peace);
				co_await vassal->get_game_data()->set_diplomacy_state(this->domain, diplomacy_state::peace);

				if (this->domain == game::get()->get_player_country()) {
					const portrait *foreign_minister_portrait = this->get_government()->get_foreign_minister_portrait();

					engine_interface::get()->add_notification("Vassal Breaks Free", foreign_minister_portrait, std::format("{}, due to the loss of standing incurred by our demotion to {} {}, our vassal, the {}, has decided to break free of our control!", this->get_form_of_address(), string::get_indefinite_article(this->get_title_name()), this->get_title_name(), vassal->get_game_data()->get_titled_name()));
				} else if (vassal == game::get()->get_player_country()) {
					const portrait *foreign_minister_portrait = vassal->get_government()->get_foreign_minister_portrait();

					engine_interface::get()->add_notification("Independence!", foreign_minister_portrait, std::format("{}, due to the loss of standing incurred by the demotion of our overlord, the {}, to {} {}, we have managed to break free of their control!", vassal->get_game_data()->get_form_of_address(), this->get_titled_name(), string::get_indefinite_article(this->get_title_name()), this->get_title_name()));
				}
			}
		}
	} else if (tier > old_tier && this->get_overlord() != nullptr && tier >= this->get_overlord()->get_game_data()->get_tier()) {
		if (this->get_overlord() == game::get()->get_player_country()) {
			const portrait *foreign_minister_portrait = this->get_overlord()->get_government()->get_foreign_minister_portrait();

			engine_interface::get()->add_notification("Vassal Breaks Free", foreign_minister_portrait, std::format("{}, due to the increase in standing incurred by the promotion of our vassal, the {}, to {} {}, they have decided to break free of our control!", this->get_overlord()->get_game_data()->get_form_of_address(), this->get_titled_name(), string::get_indefinite_article(this->get_title_name()), this->get_title_name()));
		} else if (this->domain == game::get()->get_player_country()) {
			const portrait *foreign_minister_portrait = this->get_government()->get_foreign_minister_portrait();

			engine_interface::get()->add_notification("Independence!", foreign_minister_portrait, std::format("{}, due to the increase in standing incurred by our promotion to {} {}, we have managed to break free of the control of our overlord, the {}!", this->get_form_of_address(), string::get_indefinite_article(this->get_title_name()), this->get_title_name(), this->get_overlord()->get_game_data()->get_titled_name()));
		}

		co_await this->get_overlord()->get_game_data()->set_diplomacy_state(this->domain, diplomacy_state::peace);
		co_await this->set_diplomacy_state(this->get_overlord(), diplomacy_state::peace);
	}

	if (game::get()->is_running()) {
		emit tier_changed();
	}
}

QCoro::Task<void> domain_game_data::check_tier()
{
	if (this->is_under_anarchy()) {
		co_return;
	}

	const domain_tier current_tier = this->get_tier();
	const domain_tier_data *current_tier_data = domain_tier_data::get(current_tier);
	const int domain_size = this->get_holding_count_with_vassals();
	if (domain_size >= current_tier_data->get_min_domain_size() && domain_size <= current_tier_data->get_max_domain_size()) {
		co_return;
	}

	domain_tier new_tier = domain_tier::none;

	magic_enum::enum_for_each<domain_tier>([this, domain_size, &new_tier, current_tier](const domain_tier tier) {
		if (tier == domain_tier::none) {
			return;
		}

		const domain_tier_data *tier_data = domain_tier_data::get(tier);
		if (domain_size < tier_data->get_min_domain_size()) {
			return;
		}

		if (tier > current_tier) {
			//if the tier is higher than the current tier, only allow it if the tier would still be below that of the overlord (if any)
			if (this->get_overlord() != nullptr && tier >= this->get_overlord()->get_game_data()->get_tier()) {
				return;
			}

			//if the tier is higher than the current tier, require the domain to have the appropriate tier core provinces/holdings
			const std::vector<const province *> tier_core_provinces = this->domain->get_core_provinces_for_tier(tier);
			for (const province *core_province : tier_core_provinces) {
				if (core_province->get_game_data()->get_owner() != this->domain) {
					return;
				}
			}

			const std::vector<const site *> tier_core_holdings = this->domain->get_core_holdings_for_tier(tier);
			for (const site *core_holding : tier_core_holdings) {
				if (core_holding->get_game_data()->get_owner() != this->domain) {
					return;
				}
			}
		}

		new_tier = tier;
	});

	assert_throw(new_tier != domain_tier::none);
	assert_throw(new_tier != current_tier);

	new_tier = std::max(new_tier, this->domain->get_min_tier());
	new_tier = std::min(new_tier, this->domain->get_max_tier());

	if (new_tier == current_tier) {
		co_return;
	}

	co_await this->set_tier(new_tier);

	if (this->domain == game::get()->get_player_country()) {
		const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

		engine_interface::get()->add_notification(this->get_titled_name(), interior_minister_portrait, std::format("{}, due to its recent change in size, our domain is now known as {} {}!", this->get_form_of_address(), string::get_indefinite_article(this->get_title_name()), this->get_title_name()));
	}
}

const std::string &domain_game_data::get_name() const
{
	return this->domain->get_name(this->get_government_type(), this->get_tier());
}

std::string domain_game_data::get_titled_name() const
{
	return this->domain->get_titled_name(this->get_government_type(), this->get_tier(), this->get_culture(), this->get_religion());
}

const std::string &domain_game_data::get_title_name() const
{
	return this->domain->get_title_name(this->get_government_type(), this->get_tier(), this->get_culture(), this->get_religion());
}

const std::string &domain_game_data::get_form_of_address() const
{
	assert_throw(this->get_government_type() != nullptr);
	assert_throw(this->get_government()->get_ruler() != nullptr);

	return this->get_government_type()->get_form_of_address(this->get_tier(), this->get_government()->get_ruler()->get_gender());
}

const std::string &domain_game_data::get_flag() const
{
	for (const auto &[conditional_flag, conditions] : this->domain->get_conditional_flags()) {
		assert_throw(conditions != nullptr);

		if (conditions->check(this->domain, read_only_context(this->domain))) {
			return conditional_flag;
		}
	}

	return this->domain->get_flag();
}

QCoro::Task<void> domain_game_data::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		co_return;
	}

	if (!this->domain->is_culture_allowed(culture)) {
		throw std::runtime_error(std::format("Tried to set culture \"{}\" for domain \"{}\", which does not have that culture as an allowed one.", culture->get_identifier(), this->domain->get_identifier()));
	}

	this->culture = culture;

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_population()->get_main_culture() == nullptr) {
			province->get_game_data()->set_culture(this->get_culture());
		}
	}

	if (game::get()->is_running()) {
		emit culture_changed();
	}
}

QCoro::Task<void> domain_game_data::check_culture()
{
	std::vector<const metternich::culture *> potential_cultures;

	if (this->get_government()->get_ruler() != nullptr && this->domain->is_culture_allowed(this->get_government()->get_ruler()->get_culture())) {
		//use the ruler's culture for the domain if it is allowed for it
		potential_cultures = { this->get_government()->get_ruler()->get_culture() };
	} else {
		//get the allowed culture with most population
		int64_t best_size = 0;

		for (const auto &[culture, size] : this->get_population()->get_culture_sizes()) {
			if (!this->domain->is_culture_allowed(culture)) {
				continue;
			}

			if (size < best_size) {
				continue;
			} else if (size > best_size) {
				potential_cultures.clear();
				best_size = size;
			}

			potential_cultures.push_back(culture);
		}
	}

	if (potential_cultures.empty()) {
		co_return;
	}

	const metternich::culture *chosen_culture = vector::get_random(potential_cultures);
	if (chosen_culture == this->get_culture()) {
		co_return;
	}

	co_await this->set_culture(chosen_culture);

	if (this->domain == game::get()->get_player_country()) {
		const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

		engine_interface::get()->add_notification("New State Culture", interior_minister_portrait, std::format("{}, the {} culture has taken hold of our institutions, becoming our new state culture!", this->get_form_of_address(), chosen_culture->get_name()));
	}
}

QCoro::Task<void> domain_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		co_return;
	}

	this->religion = religion;

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_population()->get_main_religion() == nullptr) {
			province->get_game_data()->set_religion(this->get_religion());
		}
	}

	if (game::get()->is_running()) {
		emit religion_changed();
	}
}

QCoro::Task<void> domain_game_data::set_overlord(const metternich::domain *overlord)
{
	if (overlord == this->get_overlord()) {
		co_return;
	}

	if (overlord != nullptr && overlord->get_game_data()->get_tier() <= this->get_tier()) {
		throw std::runtime_error(std::format("Tried to set \"{}\" as the overlord of \"{}\", but the former does not have a higher tier than the latter.", overlord->get_identifier(), this->domain->get_identifier()));
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(-this->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);

		for (const auto &[resource, count] : this->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, -count);
		}
	}

	this->overlord = overlord;

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);

		for (const auto &[resource, count] : this->get_economy()->get_resource_counts()) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, count);
		}
	} else {
		co_await this->set_subject_type(nullptr);
	}

	if (game::get()->is_running()) {
		emit overlord_changed();
	}
}

bool domain_game_data::is_vassal_of(const metternich::domain *domain) const
{
	return this->get_overlord() == domain;
}

bool domain_game_data::is_any_vassal_of(const metternich::domain *domain) const
{
	if (this->is_vassal_of(domain)) {
		return true;
	}

	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->is_any_vassal_of(domain);
	}

	return false;
}

bool domain_game_data::is_overlord_of(const metternich::domain *domain) const
{
	return domain->get_game_data()->is_vassal_of(this->domain);
}

bool domain_game_data::is_any_overlord_of(const metternich::domain *domain) const
{
	if (this->is_overlord_of(domain)) {
		return true;
	}

	const std::vector<const metternich::domain *> vassals = this->get_vassals();
	for (const metternich::domain *vassal : this->get_vassals()) {
		if (vassal->get_game_data()->is_any_overlord_of(domain)) {
			return true;
		}
	}

	return false;
}

const metternich::domain *domain_game_data::get_realm() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_realm();
	}

	return this->domain;
}

std::string domain_game_data::get_type_name() const
{
	switch (this->domain->get_type()) {
		case country_type::polity:
			if (this->get_rank() != nullptr) {
				return this->get_rank()->get_name();
			}
		case country_type::clade:
		case country_type::tribe:
			return get_country_type_name(this->domain->get_type());
		default:
			assert_throw(false);
	}

	return std::string();
}

QCoro::Task<void> domain_game_data::set_subject_type(const metternich::subject_type *subject_type)
{
	if (subject_type == this->get_subject_type()) {
		co_return;
	}

	this->subject_type = subject_type;

	if (game::get()->is_running()) {
		emit subject_type_changed();
	}

	co_await this->check_government_type();
}

QCoro::Task<void> domain_game_data::set_government_type(const metternich::government_type *government_type)
{
	if (government_type == this->get_government_type()) {
		co_return;
	}

	const metternich::government_type *old_government_type = this->get_government_type();

	if (government_type != nullptr) {
		if (this->domain->is_tribe() && !government_type->get_group()->is_tribal()) {
			throw std::runtime_error(std::format("Tried to set a non-tribal government type (\"{}\") for a tribal country (\"{}\").", government_type->get_identifier(), this->domain->get_identifier()));
		}

		if (this->domain->is_clade() && !government_type->get_group()->is_clade()) {
			throw std::runtime_error(std::format("Tried to set a non-clade government type (\"{}\") for a clade country (\"{}\").", government_type->get_identifier(), this->domain->get_identifier()));
		}
	}

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		co_await this->get_government_type()->get_modifier()->apply(this->domain, -1);
	}

	this->government_type = government_type;

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		co_await this->get_government_type()->get_modifier()->apply(this->domain, 1);
	}

	if ((old_government_type == nullptr || !old_government_type->has_regnal_numbering()) && this->get_government_type() != nullptr && this->get_government_type()->has_regnal_numbering() && this->get_government()->get_ruler() != nullptr) {
		this->historical_monarchs[game::get()->get_date()] = this->get_government()->get_ruler();
		this->get_government()->get_ruler()->get_game_data()->add_reigned_domain(this->domain);
	}

	for (const site *site : this->get_sites()) {
		site->get_game_data()->update_holding_type_name();
	}

	if (game::get()->is_running()) {
		emit government_type_changed();
	}
}

bool domain_game_data::can_have_government_type(const metternich::government_type *government_type) const
{
	if (government_type->get_required_technology() != nullptr && !this->domain->get_technology()->has_technology(government_type->get_required_technology())) {
		return false;
	}

	for (const law *forbidden_law : government_type->get_forbidden_laws()) {
		if (this->get_government()->has_law(forbidden_law)) {
			return false;
		}
	}

	if (government_type->get_conditions() != nullptr && !government_type->get_conditions()->check(this->domain, read_only_context(this->domain))) {
		return false;
	}

	return true;
}

QCoro::Task<void> domain_game_data::check_government_type()
{
	if (this->get_government_type() != nullptr && this->can_have_government_type(this->get_government_type())) {
		co_return;
	}

	std::vector<const metternich::government_type *> potential_government_types;

	for (const metternich::government_type *government_type : government_type::get_all()) {
		if (this->can_have_government_type(government_type)) {
			potential_government_types.push_back(government_type);
		}
	}

	assert_throw(!potential_government_types.empty());

	co_await this->set_government_type(vector::get_random(potential_government_types));
}

bool domain_game_data::is_tribal() const
{
	return this->get_government_type()->get_group()->is_tribal();
}

bool domain_game_data::is_clade() const
{
	return this->get_government_type()->get_group()->is_clade();
}

const dynasty *domain_game_data::get_dynasty() const
{
	if (this->get_government()->get_ruler() == nullptr) {
		return nullptr;
	}

	return this->get_government()->get_ruler()->get_dynasty();
}

QVariantList domain_game_data::get_provinces_qvariant_list() const
{
	return container::to_qvariant_list(this->get_provinces());
}

QCoro::Task<void> domain_game_data::add_province(const province *province)
{
	const bool was_alive = this->is_alive();

	co_await this->explore_province(province);

	this->provinces.push_back(province);

	this->on_province_gained(province, 1);

	const province_game_data *province_game_data = province->get_game_data();

	for (const QPoint &tile_pos : province_game_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const resource *resource = tile->get_resource();

		if (resource != nullptr) {
			if (!tile->is_resource_discovered() && !resource->is_prospectable()) {
				assert_throw(resource->get_required_technology() != nullptr);

				if (this->get_technology()->has_technology(resource->get_required_technology())) {
					co_await map::get()->set_tile_resource_discovered(tile_pos, true);
				}
			}
		}
	}

	if (province_game_data->is_country_border_province()) {
		this->border_provinces.push_back(province);
	}

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->domain) {
			continue;
		}

		//province ceased to be a country border province, remove it from the list
		if (vector::contains(this->get_border_provinces(), neighbor_province) && !neighbor_province_game_data->is_country_border_province()) {
			std::erase(this->border_provinces, neighbor_province);
		}
	}

	if (game::get()->is_loaded()) {
		this->calculate_territory_rect();
	}

	if (this->get_provinces().size() == 1) {
		game::get()->add_country(this->domain);
	}

	if (this->is_alive() && !was_alive) {
		game::get()->add_domain(this->domain);
	}

	for (const metternich::domain *domain : game::get()->get_domains()) {
		if (domain == this->domain) {
			continue;
		}

		if (!domain->get_game_data()->is_country_known(this->domain) && domain->get_game_data()->is_province_discovered(province)) {
			co_await domain->get_game_data()->add_known_country(this->domain);
		}
	}

	if (this->get_capital() == nullptr) {
		co_await this->choose_capital();
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

QCoro::Task<void> domain_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);

	if (this->get_capital_province() == province) {
		co_await this->choose_capital();
	}

	this->on_province_gained(province, -1);

	const province_game_data *province_game_data = province->get_game_data();

	std::erase(this->border_provinces, province);

	for (const metternich::province *neighbor_province : province_game_data->get_neighbor_provinces()) {
		const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->domain) {
			continue;
		}

		//province has become a country border province, add it to the list
		if (neighbor_province_game_data->is_country_border_province() && !vector::contains(this->get_border_provinces(), neighbor_province)) {
			this->border_provinces.push_back(neighbor_province);
		}
	}

	if (game::get()->is_loaded()) {
		this->calculate_territory_rect();
	}

	//remove this as a known country for other countries, if they no longer have explored tiles in this country's territory
	for (const metternich::domain *domain : game::get()->get_domains()) {
		if (domain == this->domain) {
			continue;
		}

		if (!domain->get_game_data()->is_country_known(this->domain)) {
			continue;
		}

		if (!domain->get_game_data()->is_province_discovered(province)) {
			//the other country didn't have this province discovered, so its removal from this country's territory couldn't have impacted its knowability to them anyway
			continue;
		}

		bool known_province = false;
		for (const metternich::province *loop_province : this->get_provinces()) {
			if (domain->get_game_data()->is_province_discovered(loop_province)) {
				known_province = true;
				break;
			}
		}

		bool known_site = false;
		for (const metternich::site *loop_site : this->get_sites()) {
			if (domain->get_game_data()->is_province_discovered(loop_site->get_map_data()->get_province())) {
				known_site = true;
				break;
			}
		}

		if (!known_province && !known_site) {
			domain->get_game_data()->remove_known_country(this->domain);
		}
	}

	if (this->get_provinces().empty()) {
		game::get()->remove_country(this->domain);
	}

	if (!this->is_alive()) {
		co_await game::get()->remove_domain(this->domain);
	}

	if (game::get()->is_running()) {
		emit provinces_changed();
	}
}

void domain_game_data::on_province_gained(const province *province, const int multiplier)
{
	province_game_data *province_game_data = province->get_game_data();

	if (province_game_data->is_coastal()) {
		this->coastal_province_count += 1 * multiplier;
	}

	this->change_score(province_game_data->get_level() * 100 * multiplier);
	this->change_domain_power(province_game_data->get_level() * multiplier);

	for (const auto &[resource, count] : province_game_data->get_resource_counts()) {
		this->get_economy()->change_resource_count(resource, count * multiplier);

		if (this->get_overlord() != nullptr) {
			this->get_overlord()->get_economy()->change_vassal_resource_count(resource, count * multiplier);
		}
	}

	this->change_tile_terrain_count(province_game_data->get_terrain(), static_cast<int>(province->get_map_data()->get_tiles().size() - province->get_map_data()->get_sites().size()) * multiplier);

	for (const auto &[commodity, threshold_map] : this->get_economy()->get_commodity_bonuses_for_tile_thresholds()) {
		for (const auto &[threshold, value] : threshold_map) {
			province_game_data->change_commodity_bonus_for_tile_threshold(commodity, threshold, value * multiplier);
		}
	}
}

std::vector<const province *> domain_game_data::get_accessible_provinces() const
{
	//get provinces either owned by this domain, or in which this domain has holdings
	std::vector<const province *> accessible_provinces = this->get_provinces();

	for (const site *site : this->get_sites()) {
		if (!site->is_settlement()) {
			continue;
		}

		const province *site_province = site->get_game_data()->get_province();
		if (!vector::contains(accessible_provinces, site_province)) {
			accessible_provinces.push_back(site_province);
		}
	}

	return accessible_provinces;
}

QVariantList domain_game_data::get_sites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_sites());
}

QCoro::Task<void> domain_game_data::add_site(const site *site)
{
	const bool was_alive = this->is_alive();

	co_await this->explore_province(site->get_map_data()->get_province());

	this->sites.push_back(site);

	if (this->is_alive() && !was_alive) {
		game::get()->add_domain(this->domain);
	}

	co_await this->on_site_gained(site, 1);

	if (this->get_capital() == nullptr) {
		co_await this->choose_capital();
	}

	if (game::get()->is_running()) {
		emit sites_changed();
	}
}

QCoro::Task<void> domain_game_data::remove_site(const site *site)
{
	std::erase(this->sites, site);

	if (this->get_capital() == site) {
		co_await this->choose_capital();
	}

	co_await this->on_site_gained(site, -1);

	//remove this as a known country for other countries, if they no longer have explored tiles in this country's territory
	for (const metternich::domain *domain : game::get()->get_domains()) {
		if (domain == this->domain) {
			continue;
		}

		if (!domain->get_game_data()->is_country_known(this->domain)) {
			continue;
		}

		if (!domain->get_game_data()->is_province_discovered(site->get_map_data()->get_province())) {
			//the other country didn't have this province discovered, so its removal from this country's territory couldn't have impacted its knowability to them anyway
			continue;
		}

		bool known_province = false;
		for (const metternich::province *loop_province : this->get_provinces()) {
			if (domain->get_game_data()->is_province_discovered(loop_province)) {
				known_province = true;
				break;
			}
		}

		bool known_site = false;
		for (const metternich::site *loop_site : this->get_sites()) {
			if (domain->get_game_data()->is_province_discovered(loop_site->get_map_data()->get_province())) {
				known_site = true;
				break;
			}
		}

		if (!known_province && !known_site) {
			domain->get_game_data()->remove_known_country(this->domain);
		}
	}

	if (!this->is_alive()) {
		co_await game::get()->remove_domain(this->domain);
	}

	if (game::get()->is_running()) {
		emit sites_changed();
	}
}

QCoro::Task<void> domain_game_data::on_site_gained(const site *site, const int multiplier)
{
	const site_game_data *site_game_data = site->get_game_data();

	if (site->is_settlement() && site_game_data->is_built()) {
		co_await this->change_holding_count(1 * multiplier);
		this->change_score(site_game_data->get_holding_level() * 100 * multiplier);
		this->change_domain_power(site_game_data->get_holding_level() * multiplier);

		for (const auto &[attribute, value] : this->get_site_attribute_values()) {
			co_await site->get_game_data()->change_attribute_value(attribute, value * multiplier);
		}

		for (const qunique_ptr<building_slot> &building_slot : site_game_data->get_building_slots()) {
			const building_type *building = building_slot->get_building();
			if (building != nullptr) {
				co_await this->change_settlement_building_count(building, 1 * multiplier);
			}

			const wonder *wonder = building_slot->get_wonder();
			if (wonder != nullptr) {
				co_await this->on_wonder_gained(wonder, multiplier);
			}
		}

		for (const auto &[employment_type, employment_size] : site->get_game_data()->get_employment_sizes()) {
			if (employment_type->get_domain_modifier() != nullptr) {
				co_await employment_type->get_domain_modifier()->apply(this->domain, (centesimal_int(employment_size) / employment_type->get_base_employment_size()) * multiplier);
			}
		}
	}

	const resource *site_resource = site->get_game_data()->get_resource();
	if (site_resource != nullptr) {
		if (site_resource->get_country_modifier() != nullptr) {
			co_await site_resource->get_country_modifier()->apply(this->domain, multiplier);
		}
	}

	this->change_tile_terrain_count(site->get_map_data()->get_terrain(), 1 * multiplier);
}

QCoro::Task<void> domain_game_data::set_capital(const site *capital)
{
	if (capital == this->get_capital()) {
		co_return;
	}

	if (capital != nullptr) {
		assert_throw(capital->is_settlement());
		assert_throw(this->get_provinces().empty() || capital->get_game_data()->get_province()->get_game_data()->get_owner() == this->domain);
		assert_throw(capital->get_game_data()->is_built());
	}

	const site *old_capital = this->get_capital();
	const province *old_capital_province = this->get_capital_province();

	this->capital = capital;

	if (capital != nullptr) {
		capital->get_game_data()->calculate_commodity_outputs();
		co_await capital->get_game_data()->check_building_conditions();
		if (!capital->get_game_data()->is_provincial_capital()) {
			capital->get_map_data()->get_province()->get_game_data()->choose_provincial_capital();
		}

		if (game::get()->is_loaded()) {
			this->calculate_text_rect();
			this->calculate_realm_text_rect();
		}
	}

	if (old_capital != nullptr) {
		old_capital->get_game_data()->calculate_commodity_outputs();
		co_await old_capital->get_game_data()->check_building_conditions();
		if (old_capital->get_game_data()->is_provincial_capital()) {
			old_capital->get_map_data()->get_province()->get_game_data()->choose_provincial_capital();
		}
	}

	const province *capital_province = this->get_capital_province();

	if (old_capital_province != capital_province) {
		const technology_set old_capital_technologies = old_capital_province != nullptr ? old_capital_province->get_game_data()->get_technologies() : technology_set();
		const technology_set new_capital_technologies = capital_province != nullptr ? capital_province->get_game_data()->get_technologies() : technology_set();

		for (const metternich::technology *technology : old_capital_technologies) {
			if (!new_capital_technologies.contains(technology)) {
				co_await this->get_technology()->on_technology_lost(technology);
			}
		}

		for (const metternich::technology *technology : new_capital_technologies) {
			if (!old_capital_technologies.contains(technology)) {
				co_await this->get_technology()->on_technology_added(technology);
			}
		}
	}

	emit capital_changed();
}

QCoro::Task<void> domain_game_data::choose_capital()
{
	const site *default_capital = this->domain->get_default_capital();
	if (default_capital->get_game_data()->get_owner() == this->domain && default_capital->get_game_data()->can_be_capital()) {
		co_await this->set_capital(default_capital);
		co_return;
	}

	const province *default_capital_province = default_capital->get_game_data()->get_province();
	if (default_capital_province != nullptr && default_capital_province->get_game_data()->get_owner() == this->domain && default_capital_province->get_game_data()->get_provincial_capital() != nullptr && default_capital_province->get_game_data()->get_provincial_capital()->get_game_data()->can_be_capital()) {
		co_await this->set_capital(default_capital_province->get_game_data()->get_provincial_capital());
		co_return;
	}

	const site *best_capital = nullptr;

	for (const site *site : this->get_sites()) {
		const site_game_data *site_game_data = site->get_game_data();

		if (!site_game_data->can_be_capital()) {
			continue;
		}

		if (best_capital != nullptr && !this->get_government_type()->is_holding_type_preferred(best_capital->get_game_data()->get_holding_type()) && this->get_government_type()->is_holding_type_preferred(site_game_data->get_holding_type())) {
			best_capital = nullptr;
		}

		if (best_capital != nullptr) {
			if (this->get_government_type()->is_holding_type_preferred(best_capital->get_game_data()->get_holding_type()) && !this->get_government_type()->is_holding_type_preferred(site_game_data->get_holding_type())) {
				continue;
			}

			if (best_capital->get_game_data()->is_provincial_capital() && !site_game_data->is_provincial_capital()) {
				continue;
			}

			if (best_capital->get_game_data()->get_holding_type()->get_level() >= site_game_data->get_holding_type()->get_level()) {
				continue;
			}
		}

		best_capital = site;
	}

	co_await this->set_capital(best_capital);
}

const province *domain_game_data::get_capital_province() const
{
	if (this->get_capital() != nullptr) {
		return this->get_capital()->get_game_data()->get_province();
	}

	return nullptr;
}

QCoro::Task<void> domain_game_data::change_holding_count(const int change)
{
	if (change == 0) {
		co_return;
	}

	const int old_holding_count = this->get_holding_count();

	this->holding_count += change;

	for (const auto &[building, count] : this->settlement_building_counts) {
		if (building->get_weighted_domain_modifier() != nullptr) {
			co_await building->get_weighted_domain_modifier()->apply(this->domain, centesimal_int(-count) / old_holding_count);
		}
	}

	if (this->get_holding_count() != 0) {
		for (const auto &[building, count] : this->settlement_building_counts) {
			if (building->get_weighted_domain_modifier() != nullptr) {
				//reapply the settlement building's weighted country modifier with the updated settlement count
				co_await building->get_weighted_domain_modifier()->apply(this->domain, centesimal_int(count) / this->get_holding_count());
			}
		}
	}

	if (game::get()->is_running()) {
		emit holding_count_changed();
	}
}

int domain_game_data::get_holding_count_with_vassals() const
{
	int holding_count = this->get_holding_count();

	const std::vector<const metternich::domain *> vassals = this->get_vassals();
	for (const metternich::domain *vassal : this->get_vassals()) {
		holding_count += vassal->get_game_data()->get_holding_count_with_vassals();
	}

	return holding_count;
}

bool domain_game_data::is_playable() const
{
	if (this->get_government()->get_ruler() == nullptr) {
		return false;
	}

	if (!this->get_government()->get_ruler()->get_game_data()->is_playable()) {
		return false;
	}

	if (this->is_under_anarchy()) {
		return false;
	}

	return this->domain->get_type() == country_type::polity;
}

QString domain_game_data::get_unplayable_reason() const
{
	if (this->get_government()->get_ruler() == nullptr) {
		return "You cannot play as a domain without a ruler";
	}

	if (!this->get_government()->get_ruler()->get_game_data()->is_playable()) {
		return this->get_government()->get_ruler()->get_game_data()->get_unplayable_reason();
	}

	if (this->is_under_anarchy()) {
		return "You cannot play as a domain under anarchy";
	}

	if (this->domain->get_type() != country_type::polity) {
		return "You cannot play as a non-polity domain";
	}

	return QString();
}

void domain_game_data::calculate_territory_rect()
{
	QRect territory_rect;
	this->contiguous_territory_rects.clear();

	for (const province *province : this->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();

		if (territory_rect.isNull()) {
			territory_rect = province_game_data->get_territory_rect();
		} else {
			territory_rect = territory_rect.united(province_game_data->get_territory_rect());
		}

		this->contiguous_territory_rects.push_back(province_game_data->get_territory_rect());
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (size_t i = 0; i < this->contiguous_territory_rects.size(); ++i) {
			QRect &first_territory_rect = this->contiguous_territory_rects.at(i);

			for (size_t j = i + 1; j < this->contiguous_territory_rects.size();) {
				const QRect &second_territory_rect = this->contiguous_territory_rects.at(j);

				if (first_territory_rect.intersects(second_territory_rect) || rect::is_adjacent_to(first_territory_rect, second_territory_rect)) {
					first_territory_rect = first_territory_rect.united(second_territory_rect);
					this->contiguous_territory_rects.erase(this->contiguous_territory_rects.begin() + j);
					changed = true;
				} else {
					++j;
				}
			}
		}
	}

	this->territory_rect = territory_rect;

	const QPoint &capital_pos = this->get_capital() ? this->get_capital()->get_game_data()->get_tile_pos() : this->domain->get_default_capital()->get_game_data()->get_tile_pos();
	int best_distance = std::numeric_limits<int>::max();
	for (const QRect &contiguous_territory_rect : this->get_contiguous_territory_rects()) {
		if (contiguous_territory_rect.contains(capital_pos)) {
			this->main_contiguous_territory_rect = contiguous_territory_rect;
			break;
		}

		int distance = rect::distance_to(contiguous_territory_rect, capital_pos);

		if (distance < best_distance) {
			best_distance = distance;
			this->main_contiguous_territory_rect = contiguous_territory_rect;
		}
	}

	if (!this->get_territory_rect().isNull()) {
		this->calculate_territory_rect_center();
		this->calculate_center_tile_pos();
		this->calculate_text_rect();
	}

	if (game::get()->is_running()) {
		this->domain->get_turn_data()->set_diplomatic_map_dirty(true);
	}

	if (!this->get_territory_rect().isNull()) {
		this->calculate_realm_territory_rect();
	}
}

void domain_game_data::calculate_territory_rect_center()
{
	if (this->get_provinces().empty()) {
		return;
	}

	int64_t sum_x = 0;
	int64_t sum_y = 0;
	int tile_count = 0;

	for (const province *province : this->get_provinces()) {
		const province_map_data *province_map_data = province->get_map_data();
		if (!this->get_main_contiguous_territory_rect().contains(province_map_data->get_territory_rect())) {
			continue;
		}

		const int province_tile_count = static_cast<int>(province_map_data->get_tiles().size());
		sum_x += static_cast<int64_t>(province_map_data->get_territory_rect_center().x()) * province_tile_count;
		sum_y += static_cast<int64_t>(province_map_data->get_territory_rect_center().y()) * province_tile_count;
		tile_count += province_tile_count;
	}

	this->territory_rect_center = QPoint(static_cast<int>(sum_x / tile_count), static_cast<int>(sum_y / tile_count));
}


void domain_game_data::calculate_center_tile_pos()
{
	if (this->get_provinces().empty()) {
		return;
	}

	this->center_tile_pos = this->get_territory_rect_center();

	assert_throw(this->get_center_tile_pos() != QPoint(-1, -1));

	if (map::get()->get_tile(this->get_center_tile_pos())->get_owner() != this->domain) {
		//if the center pos is not in the domain, set it to the nearest tile that is actually in the domain instead

		const QRect map_rect(QPoint(0, 0), map::get()->get_size());
		bool found_pos = false;
		int64_t best_distance = std::numeric_limits<int64_t>::max();
		QPoint best_tile_pos = this->get_center_tile_pos();

		const int max_range = std::max(16, std::max(this->get_main_contiguous_territory_rect().width(), this->get_main_contiguous_territory_rect().height()));
		for (int i = 1; i <= max_range; ++i) {
			const QRect rect(this->get_center_tile_pos() - QPoint(i, i), this->get_center_tile_pos() + QPoint(i, i));

			bool checked_on_map = false;

			rect::for_each_edge_point(rect, [this, &map_rect, &found_pos, &best_distance, &best_tile_pos, &checked_on_map](const QPoint &checked_pos) {
				if (!map_rect.contains(checked_pos)) {
					return;
				}

				checked_on_map = true;

				const metternich::domain *tile_domain = map::get()->get_tile(checked_pos)->get_owner();
				if (tile_domain != this->domain) {
					return;
				}

				const int64_t distance = point::square_distance_to(this->get_center_tile_pos(), checked_pos);
				if (distance < best_distance) {
					best_distance = distance;
					best_tile_pos = checked_pos;
					found_pos = true;
				}
			});

			if (found_pos) {
				break;
			}

			if (!checked_on_map) {
				break;
			}
		}

		if (!found_pos) {
			throw std::runtime_error(std::format("No position found for the center tile pos of domain \"{}\".", this->domain->get_identifier()));
		}

		this->center_tile_pos = best_tile_pos;
	}

	assert_throw(map::get()->get_tile(this->get_center_tile_pos())->get_owner() == this->domain);
}

QVariantList domain_game_data::get_contiguous_territory_rects_qvariant_list() const
{
	return container::to_qvariant_list(this->get_contiguous_territory_rects());
}

void domain_game_data::calculate_text_rect()
{
	this->text_rect = QRect();

	if (!this->is_alive()) {
		return;
	}

	const map *map = map::get();

	this->text_rect = this->calculate_text_rect(this->main_contiguous_territory_rect, [this, map](const QPoint &tile_pos) {
		const metternich::tile *tile = map->get_tile(tile_pos);
		return tile->get_owner() == this->domain;
	});
}

QRect domain_game_data::calculate_text_rect(const QRect &main_contiguous_territory_rect, const std::function<bool(const QPoint &)> &can_expand_func) const
{
	if (this->get_provinces().empty()) {
		return QRect();
	}

	const map *map = map::get();

	const QPoint center_pos = this->get_center_tile_pos();

	if (!map->contains(center_pos)) {
		throw std::runtime_error(std::format("Domain \"{}\" has a center pos of {}, which is not contained by the map.", this->domain->get_identifier(), point::to_string(center_pos)));
	}

	if (map->get_tile(center_pos)->get_owner() != this->domain) {
		return QRect();
	}

	QRect text_rect = QRect(center_pos, QSize(1, 1));

	bool changed = true;
	while (changed) {
		changed = false;

		bool can_expand_left = true;
		const int left_x = text_rect.left() - 1;
		for (int y = text_rect.top(); y <= text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(left_x, y);

			if (!main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_left = false;
				break;
			}

			if (!can_expand_func(adjacent_pos)) {
				can_expand_left = false;
				break;
			}
		}
		if (can_expand_left) {
			text_rect.setLeft(left_x);
			changed = true;
		}

		bool can_expand_right = true;
		const int right_x = text_rect.right() + 1;
		for (int y = text_rect.top(); y <= text_rect.bottom(); ++y) {
			const QPoint adjacent_pos(right_x, y);

			if (!main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_right = false;
				break;
			}

			if (!can_expand_func(adjacent_pos)) {
				can_expand_right = false;
				break;
			}
		}
		if (can_expand_right) {
			text_rect.setRight(right_x);
			changed = true;
		}

		bool can_expand_up = true;
		const int up_y = text_rect.top() - 1;
		for (int x = text_rect.left(); x <= text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, up_y);

			if (!main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_up = false;
				break;
			}

			if (!can_expand_func(adjacent_pos)) {
				can_expand_up = false;
				break;
			}
		}
		if (can_expand_up) {
			text_rect.setTop(up_y);
			changed = true;
		}

		bool can_expand_down = true;
		const int down_y = text_rect.bottom() + 1;
		for (int x = text_rect.left(); x <= text_rect.right(); ++x) {
			const QPoint adjacent_pos(x, down_y);

			if (!main_contiguous_territory_rect.contains(adjacent_pos)) {
				can_expand_down = false;
				break;
			}

			if (!can_expand_func(adjacent_pos)) {
				can_expand_down = false;
				break;
			}
		}
		if (can_expand_down) {
			text_rect.setBottom(down_y);
			changed = true;
		}
	}

	return text_rect;
}

void domain_game_data::calculate_realm_territory_rect()
{
	if (this->get_territory_rect().isNull()) {
		//if the territory rect hasn't been calculated yet, do so
		this->calculate_territory_rect();
		return; //we can return here, since the territory rect calculation will trigger a further realm territory rect calculation by itself
	}

	QRect territory_rect = this->get_territory_rect();
	this->realm_contiguous_territory_rects = this->get_contiguous_territory_rects();

	for (const metternich::domain *vassal : this->get_vassals()) {
		if (vassal->get_game_data()->get_realm_territory_rect().isNull()) {
			continue;
		}

		if (territory_rect.isNull()) {
			territory_rect = vassal->get_game_data()->get_realm_territory_rect();
		} else {
			territory_rect = territory_rect.united(vassal->get_game_data()->get_realm_territory_rect());
		}

		vector::merge(this->realm_contiguous_territory_rects, vassal->get_game_data()->get_realm_contiguous_territory_rects());
	}

	bool changed = true;
	while (changed) {
		changed = false;

		for (size_t i = 0; i < this->realm_contiguous_territory_rects.size(); ++i) {
			QRect &first_territory_rect = this->realm_contiguous_territory_rects.at(i);

			for (size_t j = i + 1; j < this->realm_contiguous_territory_rects.size();) {
				const QRect &second_territory_rect = this->realm_contiguous_territory_rects.at(j);

				if (first_territory_rect.intersects(second_territory_rect) || rect::is_adjacent_to(first_territory_rect, second_territory_rect)) {
					first_territory_rect = first_territory_rect.united(second_territory_rect);
					this->realm_contiguous_territory_rects.erase(this->realm_contiguous_territory_rects.begin() + j);
					changed = true;
				} else {
					++j;
				}
			}
		}
	}

	this->realm_territory_rect = territory_rect;

	const QPoint &capital_pos = this->get_capital() ? this->get_capital()->get_game_data()->get_tile_pos() : this->domain->get_default_capital()->get_game_data()->get_tile_pos();
	int best_distance = std::numeric_limits<int>::max();
	for (const QRect &contiguous_territory_rect : this->get_realm_contiguous_territory_rects()) {
		if (contiguous_territory_rect.contains(capital_pos)) {
			this->main_realm_contiguous_territory_rect = contiguous_territory_rect;
			break;
		}

		int distance = rect::distance_to(contiguous_territory_rect, capital_pos);

		if (distance < best_distance) {
			best_distance = distance;
			this->main_realm_contiguous_territory_rect = contiguous_territory_rect;
		}
	}

	this->calculate_realm_text_rect();

	if (game::get()->is_running() && this->is_independent()) {
		this->domain->get_turn_data()->set_realm_diplomatic_map_dirty(true);
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->calculate_realm_territory_rect();
	}
}

void domain_game_data::calculate_realm_text_rect()
{
	this->realm_text_rect = QRect();

	if (!this->is_alive() || !this->is_independent()) {
		return;
	}

	const map *map = map::get();

	this->realm_text_rect = this->calculate_text_rect(this->main_realm_contiguous_territory_rect, [this, map](const QPoint &tile_pos) {
		const metternich::tile *tile = map->get_tile(tile_pos);
		return tile->get_owner() != nullptr && tile->get_owner()->get_game_data()->get_realm() == this->domain;
	});
}

QVariantList domain_game_data::get_tile_terrain_counts_qvariant_list() const
{
	QVariantList counts = archimedes::map::to_qvariant_list(this->get_tile_terrain_counts());
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});
	return counts;
}

QCoro::Task<void> domain_game_data::add_known_country(const metternich::domain *other_domain)
{
	this->known_countries.insert(other_domain);

	const consulate *current_consulate = this->get_consulate(other_domain);

	const consulate *best_free_consulate = nullptr;
	for (const auto &[consulate, count] : this->free_consulate_counts) {
		if (best_free_consulate == nullptr || consulate->get_level() > best_free_consulate->get_level()) {
			best_free_consulate = consulate;
		}
	}

	if (best_free_consulate != nullptr && (current_consulate == nullptr || current_consulate->get_level() < best_free_consulate->get_level())) {
		this->set_consulate(other_domain, best_free_consulate);
	}

	if (this->get_technology()->get_gain_technologies_known_by_others_count() > 0) {
		co_await this->get_technology()->gain_technologies_known_by_others();
	}
}

diplomacy_state domain_game_data::get_diplomacy_state(const metternich::domain *other_domain) const
{
	const auto find_iterator = this->diplomacy_states.find(other_domain);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

QCoro::Task<void> domain_game_data::set_diplomacy_state(const metternich::domain *other_domain, const diplomacy_state state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_domain);

	if (state == old_state) {
		co_return;
	}

	if (is_vassalage_diplomacy_state(state)) {
		co_await this->set_overlord(other_domain);
	} else {
		if (this->get_overlord() == other_domain) {
			co_await this->set_overlord(nullptr);
		}
	}

	if (old_state != diplomacy_state::peace) {
		this->change_diplomacy_state_count(old_state, -1);
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_domain);
	} else {
		this->diplomacy_states[other_domain] = state;
		this->change_diplomacy_state_count(state, 1);
	}

	if (is_overlordship_diplomacy_state(old_state) || is_overlordship_diplomacy_state(state)) {
		if (game::get()->is_loaded()) {
			this->calculate_realm_territory_rect();
		}
	}

	if (game::get()->is_running()) {
		emit diplomacy_states_changed();

		if (is_vassalage_diplomacy_state(state) || is_vassalage_diplomacy_state(old_state)) {
			emit type_name_changed();
		}
	}
}

void domain_game_data::change_diplomacy_state_count(const diplomacy_state state, const int change)
{
	const int final_count = (this->diplomacy_state_counts[state] += change);

	if (final_count == 0) {
		this->diplomacy_state_counts.erase(state);
		this->diplomacy_state_diplomatic_map_image_promises.erase(state);
	}

	//if the change added the diplomacy state to the map, then we need to create the diplomatic map image for it
	if (game::get()->is_running() && final_count == change && !is_vassalage_diplomacy_state(state) && !is_overlordship_diplomacy_state(state)) {
		this->domain->get_turn_data()->set_diplomatic_map_diplomacy_state_dirty(state);
	}
}

QString domain_game_data::get_diplomacy_state_diplomatic_map_suffix(metternich::domain *other_domain) const
{
	if (other_domain == this->domain || this->is_any_overlord_of(other_domain) || this->is_any_vassal_of(other_domain)) {
		return "empire";
	}

	return QString::fromStdString(std::string(magic_enum::enum_name(this->get_diplomacy_state(other_domain))));
}

bool domain_game_data::at_war() const
{
	return this->diplomacy_state_counts.contains(diplomacy_state::war);
}

bool domain_game_data::can_attack(const metternich::domain *other_domain) const
{
	if (other_domain == nullptr) {
		return false;
	}

	if (other_domain == this->domain) {
		return false;
	}

	if (this->is_any_overlord_of(other_domain)) {
		return false;
	}

	if (other_domain->is_clade()) {
		return true;
	} else if (this->is_clade()) {
		return false;
	}

	switch (this->get_diplomacy_state(other_domain)) {
		case diplomacy_state::non_aggression_pact:
		case diplomacy_state::alliance:
			return false;
		case diplomacy_state::war:
			return true;
		default:
			break;
	}

	if (other_domain->get_game_data()->is_tribal() || this->is_tribal()) {
		return true;
	}

	if (other_domain->get_game_data()->is_under_anarchy() || this->is_under_anarchy()) {
		return true;
	}

	return false;
}

std::optional<diplomacy_state> domain_game_data::get_offered_diplomacy_state(const metternich::domain *other_domain) const
{
	const auto find_iterator = this->offered_diplomacy_states.find(other_domain);

	if (find_iterator != this->offered_diplomacy_states.end()) {
		return find_iterator->second;
	}

	return std::nullopt;
}

void domain_game_data::set_offered_diplomacy_state(const metternich::domain *other_domain, const std::optional<diplomacy_state> &state)
{
	const diplomacy_state old_state = this->get_diplomacy_state(other_domain);

	if (state == old_state) {
		return;
	}

	if (state.has_value()) {
		this->offered_diplomacy_states[other_domain] = state.value();
	} else {
		this->offered_diplomacy_states.erase(other_domain);
	}

	if (game::get()->is_running()) {
		emit offered_diplomacy_states_changed();
	}
}

QVariantList domain_game_data::get_consulates_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->consulates);
}

void domain_game_data::set_consulate(const metternich::domain *other_domain, const consulate *consulate)
{
	if (consulate == nullptr) {
		this->consulates.erase(other_domain);
	} else {
		this->consulates[other_domain] = consulate;

		if (other_domain->get_game_data()->get_consulate(this->domain) != consulate) {
			other_domain->get_game_data()->set_consulate(this->domain, consulate);
		}
	}

	if (game::get()->is_running()) {
		emit consulates_changed();
	}
}

int domain_game_data::get_opinion_of(const metternich::domain *other) const
{
	int opinion = this->get_base_opinion(other);

	for (const auto &[modifier, duration] : this->get_opinion_modifiers_for(other)) {
		opinion += modifier->get_value();
	}

	opinion = std::clamp(opinion, domain::min_opinion, domain::max_opinion);

	return opinion;
}

void domain_game_data::set_base_opinion(const metternich::domain *other, const int opinion)
{
	assert_throw(other != this->domain);

	if (opinion == this->get_base_opinion(other)) {
		return;
	}

	if (opinion < domain::min_opinion) {
		this->set_base_opinion(other, domain::min_opinion);
		return;
	} else if (opinion > domain::max_opinion) {
		this->set_base_opinion(other, domain::max_opinion);
		return;
	}

	if (opinion == 0) {
		this->base_opinions.erase(other);
	} else {
		this->base_opinions[other] = opinion;
	}
}

void domain_game_data::add_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier, const int duration)
{
	this->opinion_modifiers[other][modifier] = std::max(this->opinion_modifiers[other][modifier], duration);
}

void domain_game_data::remove_opinion_modifier(const metternich::domain *other, const opinion_modifier *modifier)
{
	opinion_modifier_map<int> &opinion_modifiers = this->opinion_modifiers[other];
	opinion_modifiers.erase(modifier);

	if (opinion_modifiers.empty()) {
		this->opinion_modifiers.erase(other);
	}
}

std::vector<const metternich::domain *> domain_game_data::get_vassals() const
{
	std::vector<const metternich::domain *> vassals;

	for (const auto &[domain, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			vassals.push_back(domain);
		}
	}

	return vassals;
}

QVariantList domain_game_data::get_vassals_qvariant_list() const
{
	return container::to_qvariant_list(this->get_vassals());
}

QVariantList domain_game_data::get_subject_type_counts_qvariant_list() const
{
	std::map<const metternich::subject_type *, int> subject_type_counts;

	for (const auto &[country, diplomacy_state] : this->diplomacy_states) {
		if (is_overlordship_diplomacy_state(diplomacy_state)) {
			assert_throw(country->get_game_data()->get_subject_type() != nullptr);
			++subject_type_counts[country->get_game_data()->get_subject_type()];
		}
	}

	QVariantList counts = archimedes::map::to_qvariant_list(subject_type_counts);
	std::sort(counts.begin(), counts.end(), [](const QVariant &lhs, const QVariant &rhs) {
		return lhs.toMap().value("value").toInt() > rhs.toMap().value("value").toInt();
	});

	return counts;
}

std::vector<const metternich::domain *> domain_game_data::get_neighbor_countries() const
{
	std::vector<const metternich::domain *> neighbor_countries;

	for (const province *province : this->get_border_provinces()) {
		for (const metternich::province *neighbor_province : province->get_game_data()->get_neighbor_provinces()) {
			const metternich::province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
			if (neighbor_province_game_data->get_owner() == this->domain) {
				continue;
			}

			if (neighbor_province_game_data->get_owner() == nullptr) {
				continue;
			}

			if (vector::contains(neighbor_countries, neighbor_province_game_data->get_owner())) {
				continue;
			}

			neighbor_countries.push_back(neighbor_province_game_data->get_owner());
		}
	}

	return neighbor_countries;
}

const QColor &domain_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->domain->get_color();
}

QImage domain_game_data::prepare_diplomatic_map_image() const
{
	assert_throw(this->territory_rect.width() > 0);
	assert_throw(this->territory_rect.height() > 0);

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = QSize((this->territory_rect.width() * tile_scale).to_ceil_int(), (this->territory_rect.height() * tile_scale).to_ceil_int());
	} else {
		image_size = this->territory_rect.size();
	}

	QImage image(image_size, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

QImage domain_game_data::finalize_diplomatic_map_image(QImage &&image)
{
	assert_throw(!image.isNull());

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();

	if (tile_scale > 1) {
		QImage scaled_image;

		scaled_image = image::scale<QImage::Format_ARGB32>(image, centesimal_int(tile_scale), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});

		image = std::move(scaled_image);
	}

	std::vector<QPoint> border_pixels;

	for (int x = 0; x < image.width(); ++x) {
		for (int y = 0; y < image.height(); ++y) {
			const QPoint pixel_pos(x, y);
			const QColor pixel_color = image.pixelColor(pixel_pos);

			if (pixel_color.alpha() == 0) {
				continue;
			}

			if (pixel_pos.x() == 0 || pixel_pos.y() == 0 || pixel_pos.x() == (image.width() - 1) || pixel_pos.y() == (image.height() - 1)) {
				border_pixels.push_back(pixel_pos);
				continue;
			}

			if (pixel_color.alpha() != 255) {
				//blended color
				border_pixels.push_back(pixel_pos);
				continue;
			}

			const QPoint north_pos = pixel_pos + QPoint(0, -1);
			const QPoint east_pos = pixel_pos + QPoint(1, 0);
			const bool is_border_pixel = image.pixelColor(north_pos).alpha() == 0 || image.pixelColor(east_pos).alpha() == 0;

			if (is_border_pixel) {
				border_pixels.push_back(pixel_pos);
			}
		}
	}

	const QColor &border_pixel_color = defines::get()->get_country_border_color();

	for (const QPoint &border_pixel_pos : border_pixels) {
		image.setPixelColor(border_pixel_pos, border_pixel_color);
	}

	return image;
}

void domain_game_data::create_diplomatic_map_image()
{
	if (this->get_provinces().empty()) {
		return;
	}

	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_diplomatic_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->territory_rect.topLeft() * tile_scale;

	const QSize image_size = diplomatic_map_image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->territory_rect.topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			diplomatic_map_image.setPixelColor(pixel_pos, color);
			selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomatic_map_image_promise = promise;
	this->diplomatic_map_image_promise->start();
	assert_throw(!diplomatic_map_image.isNull());
	QThreadPool::globalInstance()->start([promise, image = std::move(diplomatic_map_image)]() mutable {
		promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> selected_promise = std::make_shared<QPromise<QImage>>();
	this->selected_diplomatic_map_image_promise = selected_promise;
	this->selected_diplomatic_map_image_promise->start();
	assert_throw(!selected_diplomatic_map_image.isNull());
	QThreadPool::globalInstance()->start([selected_promise, image = std::move(selected_diplomatic_map_image)]() mutable {
		selected_promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		selected_promise->finish();
	});

	this->diplomatic_map_image_rect = QRect(top_left, image_size);

	this->create_diplomatic_map_mode_image(diplomatic_map_mode::diplomatic);
	this->create_diplomacy_state_diplomatic_map_image(diplomacy_state::peace);

	for (const auto &[diplomacy_state, count] : this->get_diplomacy_state_counts()) {
		if (!is_vassalage_diplomacy_state(diplomacy_state) && !is_overlordship_diplomacy_state(diplomacy_state)) {
			this->create_diplomacy_state_diplomatic_map_image(diplomacy_state);
		}
	}

	this->create_diplomatic_map_mode_image(diplomatic_map_mode::terrain);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::cultural);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::religious);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::trade_zone);
	this->create_diplomatic_map_mode_image(diplomatic_map_mode::temple);

	emit diplomatic_map_image_changed();
}

QImage domain_game_data::prepare_realm_diplomatic_map_image() const
{
	assert_throw(this->realm_territory_rect.width() > 0);
	assert_throw(this->realm_territory_rect.height() > 0);

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	QSize image_size;
	if (tile_scale < 1) {
		image_size = QSize((this->realm_territory_rect.width() * tile_scale).to_ceil_int(), (this->realm_territory_rect.height() * tile_scale).to_ceil_int());
	} else {
		image_size = this->realm_territory_rect.size();
	}

	QImage image(image_size, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);

	return image;
}

void domain_game_data::create_realm_diplomatic_map_image()
{
	if (!this->is_independent() || this->get_provinces().empty()) {
		return;
	}

	if (this->get_vassals().empty() && this->get_diplomatic_map_image_promise() != nullptr && this->get_selected_diplomatic_map_image_promise() != nullptr) {
		this->realm_diplomatic_map_image_promise = this->diplomatic_map_image_promise;
		this->selected_realm_diplomatic_map_image_promise = this->selected_diplomatic_map_image_promise;
		this->realm_diplomatic_map_image_rect = this->get_diplomatic_map_image_rect();
		return;
	}

	const map *map = map::get();

	QImage diplomatic_map_image = this->prepare_realm_diplomatic_map_image();
	QImage selected_diplomatic_map_image = diplomatic_map_image;

	const QColor &color = this->get_diplomatic_map_color();
	const QColor &selected_color = defines::get()->get_selected_country_color();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->realm_territory_rect.topLeft() * tile_scale;

	const QSize image_size = diplomatic_map_image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->realm_territory_rect.topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() == nullptr || tile->get_owner()->get_game_data()->get_realm() != this->domain) {
				continue;
			}

			diplomatic_map_image.setPixelColor(pixel_pos, color);
			selected_diplomatic_map_image.setPixelColor(pixel_pos, selected_color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->realm_diplomatic_map_image_promise = promise;
	this->realm_diplomatic_map_image_promise->start();
	QThreadPool::globalInstance()->start([promise, image = std::move(diplomatic_map_image)]() mutable {
		promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});

	std::shared_ptr<QPromise<QImage>> selected_promise = std::make_shared<QPromise<QImage>>();
	this->selected_realm_diplomatic_map_image_promise = selected_promise;
	this->selected_realm_diplomatic_map_image_promise->start();
	QThreadPool::globalInstance()->start([selected_promise, image = std::move(selected_diplomatic_map_image)]() mutable {
		selected_promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		selected_promise->finish();
	});

	this->realm_diplomatic_map_image_rect = QRect(top_left, image_size);

	emit realm_diplomatic_map_image_changed();
}

void domain_game_data::create_diplomatic_map_mode_image(const diplomatic_map_mode mode)
{
	static const QColor empty_color(Qt::black);
	static constexpr QColor diplomatic_self_color(170, 148, 214);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->territory_rect.topLeft() * tile_scale;

	const QSize image_size = image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->territory_rect.topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			const QColor *color = nullptr;

			switch (mode) {
				case diplomatic_map_mode::diplomatic:
					color = &diplomatic_self_color;
					break;
				case diplomatic_map_mode::terrain:
					color = &tile->get_province()->get_map_data()->get_terrain()->get_color();
					break;
				case diplomatic_map_mode::cultural: {
					const metternich::culture *culture = tile->get_province()->get_game_data()->get_culture();

					if (culture != nullptr) {
						color = &culture->get_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::religious: {
					const metternich::religion *religion = tile->get_province()->get_game_data()->get_religion();

					if (religion != nullptr) {
						color = &religion->get_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::trade_zone: {
					const metternich::domain *trade_zone_domain = tile->get_province()->get_game_data()->get_trade_zone_domain();

					if (trade_zone_domain != nullptr) {
						color = &trade_zone_domain->get_game_data()->get_diplomatic_map_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
				case diplomatic_map_mode::temple: {
					const metternich::domain *temple_domain = tile->get_province()->get_game_data()->get_temple_domain();

					if (temple_domain != nullptr) {
						color = &temple_domain->get_game_data()->get_diplomatic_map_color();
					} else {
						color = &defines::get()->get_map_blank_color();
					}
					break;
				}
			}

			image.setPixelColor(pixel_pos, *color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomatic_map_mode_image_promises[mode] = promise;
	promise->start();

	QThreadPool::globalInstance()->start([promise, image = std::move(image)]() mutable {
		promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});
}

void domain_game_data::create_diplomacy_state_diplomatic_map_image(const diplomacy_state state)
{
	static const QColor empty_color(Qt::black);

	const map *map = map::get();

	QImage image = this->prepare_diplomatic_map_image();

	const decimillesimal_int &tile_scale = map::get()->get_diplomatic_map_tile_scale();
	const QPoint top_left = this->territory_rect.topLeft() * tile_scale;

	const QSize image_size = image.size();

	//normalize the tile top left
	const QPoint tile_top_left = this->territory_rect.topLeft() * tile_scale / tile_scale;

	for (int x = 0; x < image_size.width(); ++x) {
		for (int y = 0; y < image_size.height(); ++y) {
			const QPoint pixel_pos = QPoint(x, y);
			const QPoint relative_tile_pos = tile_scale < 1 ? pixel_pos / tile_scale : pixel_pos;
			const tile *tile = map->get_tile(tile_top_left + relative_tile_pos);

			if (tile->get_owner() != this->domain) {
				continue;
			}

			const QColor &color = defines::get()->get_diplomacy_state_color(state);

			image.setPixelColor(pixel_pos, color);
		}
	}

	std::shared_ptr<QPromise<QImage>> promise = std::make_shared<QPromise<QImage>>();
	this->diplomacy_state_diplomatic_map_image_promises[state] = promise;
	promise->start();

	QThreadPool::globalInstance()->start([promise, image = std::move(image)]() mutable {
		promise->addResult(domain_game_data::finalize_diplomatic_map_image(std::move(image)));
		promise->finish();
	});
}

QVariantList domain_game_data::get_attribute_values_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_attribute_values());
}

QCoro::Task<void> domain_game_data::change_attribute_value(const domain_attribute *attribute, const int change)
{
	if (change == 0) {
		co_return;
	}

	const int old_value = this->get_attribute_value(attribute);

	const int new_value = (this->attribute_values[attribute] += change);

	if (new_value == 0) {
		this->attribute_values.erase(attribute);
	}

	this->change_score(change * 10);

	if (change > 0) {
		for (int i = old_value + 1; i <= new_value; ++i) {
			const modifier<const metternich::domain> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				co_await value_modifier->apply(this->domain);
			}
		}
	} else {
		for (int i = old_value; i > new_value; --i) {
			const modifier<const metternich::domain> *value_modifier = attribute->get_value_modifier(i);
			if (value_modifier != nullptr) {
				co_await value_modifier->remove(this->domain);
			}
		}
	}

	if (game::get()->is_running()) {
		emit attribute_values_changed();
	}
}

bool domain_game_data::do_attribute_check(const domain_attribute *attribute, const int roll_modifier, int *roll_result_output) const
{
	static constexpr dice check_dice(1, 20);

	const int roll_result = random::get()->roll_dice(check_dice);

	if (roll_result_output != nullptr) {
		*roll_result_output = roll_result;
	}

	//there should always be at least a 5% chance of failure and a 5% chance of success
	if (roll_result == check_dice.get_sides()) {
		//e.g. if a 20 is rolled for a d20 roll
		return false;
	} else if (roll_result == 1) {
		return true;
	}

	const int attribute_value = this->get_attribute_value(attribute);
	const int modified_attribute_value = attribute_value + roll_modifier + this->get_attribute_check_control_modifier() - this->get_effective_unrest();

	return roll_result <= modified_attribute_value;
}

int domain_game_data::get_attribute_check_chance(const domain_attribute *attribute, const int roll_modifier) const
{
	assert_throw(attribute != nullptr);

	static constexpr dice check_dice(1, 20);

	int chance = this->get_attribute_value(attribute);
	chance += roll_modifier + this->get_attribute_check_control_modifier() - this->get_effective_unrest();

	if (check_dice.get_sides() != 100) {
		chance *= 100;
		chance /= check_dice.get_sides();
	}

	chance = std::min(chance, 95);
	chance = std::max(chance, 5);

	return chance;
}

int domain_game_data::get_attribute_check_control_modifier() const
{
	const int domain_size = this->get_holding_count();
	return -domain_size;
}

QVariantList domain_game_data::get_site_attribute_values_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_site_attribute_values());
}

QCoro::Task<void> domain_game_data::change_site_attribute_value(const site_attribute *attribute, const int change)
{
	if (change == 0) {
		co_return;
	}

	const int new_value = (this->site_attribute_values[attribute] += change);

	if (new_value == 0) {
		this->site_attribute_values.erase(attribute);
	}

	for (const site *site : this->get_sites()) {
		if (!site->is_settlement() || !site->get_game_data()->is_built()) {
			continue;
		}

		co_await site->get_game_data()->change_attribute_value(attribute, change);
	}

	if (game::get()->is_running()) {
		emit site_attribute_values_changed();
	}
}

void domain_game_data::set_consumption(const int consumption)
{
	if (consumption == this->get_consumption()) {
		return;
	}

	this->consumption = consumption;

	emit consumption_changed();
}

void domain_game_data::set_unrest(const int unrest)
{
	if (unrest == this->get_unrest()) {
		return;
	}

	this->unrest = unrest;

	emit unrest_changed();
}

void domain_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->score += change;

	emit score_changed();
}

void domain_game_data::change_economic_score(const int change)
{
	if (change == 0) {
		return;
	}

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(-this->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);
	}

	this->economic_score += change;

	this->change_score(change);

	if (this->get_overlord() != nullptr) {
		this->get_overlord()->get_game_data()->change_economic_score(this->get_economic_score() * this->get_subject_type()->get_wealth_tribute_rate() / 100);
	}
}

void domain_game_data::change_military_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->military_score += change;

	this->change_score(change);
}

void domain_game_data::change_domain_power(const int change)
{
	if (change == 0) {
		return;
	}

	this->domain_power += change;

	if (game::get()->is_running()) {
		emit domain_power_changed();
	}
}

const population_class *domain_game_data::get_default_population_class() const
{
	if (this->is_tribal() || this->is_clade()) {
		return defines::get()->get_default_tribal_population_class();
	} else {
		return defines::get()->get_default_population_class();
	}
}

void domain_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void domain_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void domain_game_data::on_population_unit_gained(const population_unit *population_unit, const int multiplier)
{
	Q_UNUSED(population_unit);

	this->change_food_consumption(multiplier);

	/*
	if (type->get_country_modifier() != nullptr) {
		const int population_type_count = this->get_population()->get_type_count(type);
		const int old_population_type_count = population_type_count - change;
		const centesimal_int &type_modifier_multiplier = this->get_population_type_modifier_multiplier(type);
		const centesimal_int &max_total_modifier_multiplier = type->get_max_modifier_multiplier();

		type->get_country_modifier()->apply(this->domain, -centesimal_int::min(old_population_type_count * type_modifier_multiplier, max_total_modifier_multiplier));
		type->get_country_modifier()->apply(this->domain, centesimal_int::min(population_type_count * type_modifier_multiplier, max_total_modifier_multiplier));
	}
	*/
}

phenotype_map<int64_t> domain_game_data::get_phenotype_weights() const
{
	phenotype_map<int64_t> phenotype_weights = this->get_population()->get_phenotype_sizes_for_culture(this->get_culture());

	if (phenotype_weights.empty()) {
		phenotype_weights = this->get_culture()->get_phenotype_weights();
	}

	return phenotype_weights;
}

void domain_game_data::set_population_growth(const int growth)
{
	if (growth == this->get_population_growth()) {
		return;
	}

	this->population_growth = growth;

	if (game::get()->is_running()) {
		emit population_growth_changed();
	}
}

QCoro::Task<void> domain_game_data::grow_population()
{
	if (this->population_units.empty()) {
		throw std::runtime_error("Tried to grow population in a country which has no pre-existing population.");
	}

	std::vector<population_unit *> potential_base_population_units = this->population_units;

	assert_throw(!potential_base_population_units.empty());

	const population_unit *population_unit = vector::get_random(potential_base_population_units);
	const metternich::culture *culture = population_unit->get_culture();
	const metternich::religion *religion = population_unit->get_religion();
	const phenotype *phenotype = population_unit->get_phenotype();

	const site *site = population_unit->get_site();

	const population_type *population_type = culture->get_population_class_type(site->get_game_data()->get_default_population_class());

	const int64_t population_size = 100;
	co_await site->get_game_data()->change_population(population_type, culture, religion, phenotype, nullptr, population_size, population_unit->get_literacy_rate(), 0, true);

	this->change_population_growth(-defines::get()->get_population_growth_threshold());
}

QCoro::Task<void> domain_game_data::decrease_population(const bool change_population_growth)
{
	//disband population unit, if possible
	if (!this->population_units.empty()) {
		population_unit *population_unit = this->choose_starvation_population_unit();
		if (population_unit != nullptr) {
			if (change_population_growth) {
				this->change_population_growth(1);
			}
			co_await population_unit->get_site()->get_game_data()->pop_population_unit(population_unit, true);
			co_return;
		}
	}

	assert_throw(false);
}

population_unit *domain_game_data::choose_starvation_population_unit()
{
	std::vector<population_unit *> population_units;

	bool found_non_food_producer = false;
	bool found_producer = false;
	int64_t lowest_output_value = std::numeric_limits<int>::max();
	for (population_unit *population_unit : this->get_population_units()) {
		if (population_unit->get_site()->is_settlement() && population_unit->get_site()->get_game_data()->get_population_unit_count() == 1) {
			//do not remove a settlement's last population unit
			continue;
		}

		const bool is_non_food_producer = !population_unit->is_food_producer();
		if (found_non_food_producer && !is_non_food_producer) {
			continue;
		} else if (!found_non_food_producer && is_non_food_producer) {
			found_non_food_producer = true;
			population_units.clear();
		}

		const bool is_producer = population_unit->get_type()->get_output_commodity() != nullptr;
		if (found_producer && !is_producer) {
			continue;
		} else if (!found_producer && is_producer) {
			found_producer = true;
			lowest_output_value = population_unit->get_type()->get_output_value();
			population_units.clear();
		}

		if (population_unit->get_type()->get_output_commodity() != nullptr) {
			if (population_unit->get_type()->get_output_value() > lowest_output_value) {
				continue;
			} else if (population_unit->get_type()->get_output_value() < lowest_output_value) {
				lowest_output_value = population_unit->get_type()->get_output_value();
				population_units.clear();
			}
		}

		population_units.push_back(population_unit);
	}

	if (population_units.empty()) {
		return nullptr;
	}

	return vector::get_random(population_units);
}

const icon *domain_game_data::get_population_type_small_icon(const population_type *type) const
{
	icon_map<int> icon_counts;

	for (const auto &population_unit : this->population_units) {
		if (population_unit->get_type() != type) {
			continue;
		}

		++icon_counts[population_unit->get_small_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	return best_icon;
}

int domain_game_data::get_net_food_consumption() const
{
	int food_consumption = this->get_food_consumption();

	for (const province *province : this->get_provinces()) {
		for (const site *site : province->get_game_data()->get_sites()) {
			if (!site->get_game_data()->can_have_population()) {
				continue;
			}

			if (!site->get_game_data()->is_built()) {
				continue;
			}

			food_consumption -= site->get_game_data()->get_free_food_consumption();
		}
	}

	return food_consumption;
}

int64_t domain_game_data::get_available_food() const
{
	return this->get_economy()->get_stored_food() - this->get_net_food_consumption();
}

bool domain_game_data::has_building(const building_type *building) const
{
	return this->get_settlement_building_count(building) > 0;
}

bool domain_game_data::has_building_or_better(const building_type *building) const
{
	if (this->has_building(building)) {
		return true;
	}

	for (const building_type *derived_building : building->get_derived_buildings()) {
		if (this->has_building_or_better(derived_building)) {
			return true;
		}
	}

	return false;
}

QCoro::Task<void> domain_game_data::change_settlement_building_count(const building_type *building, const int change)
{
	if (change == 0) {
		co_return;
	}

	const int old_count = this->get_settlement_building_count(building);

	const int count = (this->settlement_building_counts[building] += change);

	if (count < 0) {
		throw std::runtime_error(std::format("The settlement building count for country \"{}\" for building \"{}\" is negative ({}).", this->domain->get_identifier(), building->get_identifier(), change));
	}

	if (count == 0) {
		this->settlement_building_counts.erase(building);
	}

	if (building->get_weighted_domain_modifier() != nullptr && this->get_holding_count() != 0) {
		//reapply the settlement building's weighted country modifier with the updated count
		co_await building->get_weighted_domain_modifier()->apply(this->domain, centesimal_int(-old_count) / this->get_holding_count());
		co_await building->get_weighted_domain_modifier()->apply(this->domain, centesimal_int(count) / this->get_holding_count());
	}

	if (building->get_domain_modifier() != nullptr) {
		co_await building->get_domain_modifier()->apply(this->domain, change);
	}

	if (game::get()->is_running()) {
		emit settlement_building_counts_changed();
	}
}

QCoro::Task<void> domain_game_data::on_wonder_gained(const wonder *wonder, const int multiplier)
{
	if (wonder->get_country_modifier() != nullptr) {
		co_await wonder->get_country_modifier()->apply(this->domain, multiplier);
	}

	if (multiplier > 0) {
		game::get()->set_wonder_country(wonder, this->domain);
	} else if (multiplier < 0 && game::get()->get_wonder_country(wonder) == this->domain) {
		game::get()->set_wonder_country(wonder, nullptr);
	}
}

QCoro::Task<bool> domain_game_data::choose_construction()
{
	std::vector<std::variant<building_slot *, const province *>> buildable_locations;

	for (const province *province : this->get_provinces()) {
		if (province->get_game_data()->get_under_construction_pathway() != nullptr) {
			continue;
		}

		if (province->get_game_data()->get_buildable_pathway() != nullptr) {
			buildable_locations.push_back(province);
		}
	}
	for (const site *site : this->get_sites()) {
		if (site->is_settlement() && site->get_game_data()->is_built()) {
			for (const auto &building_slot : site->get_game_data()->get_building_slots()) {
				if (!building_slot->is_available()) {
					continue;
				}

				if (building_slot->get_under_construction_building() != nullptr) {
					continue;
				}

				const building_type *buildable_building = building_slot->get_buildable_building();
				if (buildable_building != nullptr) {
					buildable_locations.push_back(building_slot.get());
				}
			}
		}
	}

	if (buildable_locations.empty()) {
		co_return false;
	}

	vector::shuffle(buildable_locations);
	static constexpr size_t max_choosable_constructions = 5;
	buildable_locations.resize(std::min(buildable_locations.size(), max_choosable_constructions));

	if (this->is_ai()) {
		std::variant<building_slot *, const province *> chosen_buildable_location = vector::get_random(buildable_locations);
		if (std::holds_alternative<building_slot *>(chosen_buildable_location)) {
			building_slot *building_slot = std::get<metternich::building_slot *>(chosen_buildable_location);
			building_slot->build_building(building_slot->get_buildable_building());
		} else if (std::holds_alternative<const province *>(chosen_buildable_location)) {
			const province *province = std::get<const metternich::province *>(chosen_buildable_location);
			province->get_game_data()->build_pathway(province->get_game_data()->get_buildable_pathway());
		} else {
			assert_throw(false);
		}
	} else {
		this->construction_chosen_promise = std::make_unique<QPromise<void>>();
		const QFuture<void> future = this->construction_chosen_promise->future();
		this->construction_chosen_promise->start();

		emit engine_interface::get()->construction_choosable(container::to_qvariant_list(buildable_locations));
		co_await future;
	}

	this->construction_chosen_promise.reset();

	co_return true;
}

void domain_game_data::set_max_current_constructions(const int max)
{
	if (max == this->get_max_current_constructions()) {
		return;
	}

	this->max_current_constructions = max;

	emit max_current_constructions_changed();
}

std::vector<building_item_slot *> domain_game_data::get_item_slots() const
{
	std::vector<building_item_slot *> item_slots;

	for (const province *accessible_province : this->get_accessible_provinces()) {
		for (const site *site : accessible_province->get_game_data()->get_settlement_sites()) {
			vector::merge(item_slots, site->get_game_data()->get_item_slots());
		}
	}

	return item_slots;
}

QVariantList domain_game_data::get_item_slots_qvariant_list() const
{
	const std::map<item_key, std::vector<building_item_slot *>> item_slot_map = building_item_slot::item_slots_to_map(this->get_item_slots());

	QVariantList qvariant_list;

	for (const auto &[item_key, item_slots] : item_slot_map) {
		QVariantMap qvariant_map;

		qvariant_map["key"] = item_key.to_qvariant_map();
		qvariant_map["value"] = container::to_qvariant_list(item_slots);

		qvariant_list.push_back(std::move(qvariant_map));
	}

	return qvariant_list;
}

bool domain_game_data::can_declare_war_on(const metternich::domain *other_domain) const
{
	if (!this->domain->can_declare_war()) {
		return false;
	}

	if (this->get_overlord() != nullptr) {
		return other_domain == this->get_overlord();
	}

	return true;
}

QVariantList domain_game_data::get_ideas_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_ideas());
}

const idea *domain_game_data::get_idea(const idea_slot *slot) const
{
	const data_entry_map<idea_slot, const idea *> &ideas = this->get_ideas(slot->get_idea_type());
	const auto find_iterator = ideas.find(slot);

	if (find_iterator != ideas.end()) {
		return find_iterator->second;
	}

	return nullptr;
}

void domain_game_data::set_idea(const idea_slot *slot, const idea *idea)
{
	const metternich::idea *old_idea = this->get_idea(slot);

	if (idea == old_idea) {
		return;
	}

	if (old_idea != nullptr) {
		old_idea->apply_modifier(this->domain, -1);
	}

	if (idea != nullptr) {
		this->ideas[slot->get_idea_type()][slot] = idea;
	} else {
		this->ideas[slot->get_idea_type()].erase(slot);
		if (this->ideas[slot->get_idea_type()].empty()) {
			this->ideas.erase(slot->get_idea_type());
		}
	}

	if (idea != nullptr) {
		idea->apply_modifier(this->domain, 1);
	}

	if (game::get()->is_running()) {
		emit ideas_changed();

		if (this->domain == game::get()->get_player_country() && idea != nullptr) {
			const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

			switch (slot->get_idea_type()) {
				case idea_type::deity:
					engine_interface::get()->add_notification(std::format("{} Worshiped", idea->get_cultural_name(this->get_culture())), interior_minister_portrait, std::format("The cult of {} has become widespread in our nation!\n\n{}", idea->get_cultural_name(this->get_culture()), idea->get_modifier_string(this->domain)));
					break;
				default:
					assert_throw(false);
			}
		}
	}
}

QVariantList domain_game_data::get_appointed_ideas_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_appointed_ideas());
}

const idea *domain_game_data::get_appointed_idea(const idea_slot *slot) const
{
	const data_entry_map<idea_slot, const idea *> &appointed_ideas = this->get_appointed_ideas(slot->get_idea_type());
	const auto find_iterator = appointed_ideas.find(slot);

	if (find_iterator != appointed_ideas.end()) {
		return find_iterator->second;
	}

	return nullptr;
}

void domain_game_data::set_appointed_idea(const idea_slot *slot, const idea *idea)
{
	const metternich::idea *old_idea = this->get_appointed_idea(slot);

	if (idea == old_idea) {
		return;
	}

	if (idea != nullptr) {
		const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(idea);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->get_economy()->change_stored_commodity(commodity, -cost);
		}

		this->appointed_ideas[slot->get_idea_type()][slot] = idea;
	} else {
		this->appointed_ideas[slot->get_idea_type()].erase(slot);
		if (this->appointed_ideas[slot->get_idea_type()].empty()) {
			this->appointed_ideas.erase(slot->get_idea_type());
		}
	}

	if (old_idea != nullptr) {
		const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(old_idea);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->get_economy()->change_stored_commodity(commodity, cost);
		}
	}

	if (game::get()->is_running()) {
		emit appointed_ideas_changed();
	}
}

void domain_game_data::check_idea(const idea_slot *slot)
{
	if (this->is_under_anarchy()) {
		this->set_idea(slot, nullptr);
		return;
	}

	//process appointment, if any
	const idea *appointed_idea = this->get_appointed_idea(slot);
	if (appointed_idea != nullptr && this->can_have_idea(slot, appointed_idea)) {
		this->set_idea(slot, appointed_idea);
	}

	//remove research organizations if they have become obsolete
	const idea *old_idea = this->get_idea(slot);
	if (old_idea != nullptr && old_idea->get_obsolescence_technology() != nullptr && this->get_technology()->has_technology(old_idea->get_obsolescence_technology())) {
		this->set_idea(slot, nullptr);

		if (game::get()->is_running()) {
			if (this->domain == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_government()->get_interior_minister_portrait();

				switch (slot->get_idea_type()) {
					case idea_type::deity:
						engine_interface::get()->add_notification(std::format("{} No Longer Worshiped", old_idea->get_cultural_name(this->get_culture())), interior_minister_portrait, std::format("{}, despite a long and proud history of being worshiped in our nation, the cult of {} has lost favor amongst our people, and declined to nothingness.", this->get_form_of_address(), old_idea->get_cultural_name(this->get_culture())));
						break;
					default:
						assert_throw(false);
				}
			}
		}
	}
}

void domain_game_data::check_ideas()
{
	magic_enum::enum_for_each<idea_type>([this](const idea_type idea_type) {
		const std::vector<const idea_slot *> idea_slots = this->get_available_idea_slots(idea_type);
		for (const idea_slot *idea_slot : idea_slots) {
			this->check_idea(idea_slot);
		}

		const data_entry_map<idea_slot, const idea *> ideas = this->get_ideas(idea_type);
		for (const auto &[slot, slot_idea] : ideas) {
			if (!vector::contains(idea_slots, slot)) {
				this->set_idea(slot, nullptr);
			}
		}
	});

	this->appointed_ideas.clear();
}

std::vector<const idea *> domain_game_data::get_appointable_ideas(const idea_slot *slot) const
{
	std::vector<const idea *> potential_ideas;

	switch (slot->get_idea_type()) {
		case idea_type::deity:
			for (const deity *deity : deity::get_all()) {
				potential_ideas.push_back(deity);
			}
			break;
		default:
			assert_throw(false);
	}

	std::erase_if(potential_ideas, [this, slot](const idea *idea) {
		if (!this->can_gain_idea(slot, idea)) {
			return true;
		}

		return false;
	});

	return potential_ideas;
}

QVariantList domain_game_data::get_appointable_ideas_qvariant_list(const idea_slot *slot) const
{
	return container::to_qvariant_list(this->get_appointable_ideas(slot));
}

const idea *domain_game_data::get_best_idea(const idea_slot *slot)
{
	std::vector<const idea *> potential_ideas;
	int best_skill = 0;

	for (const idea *idea : this->get_appointable_ideas(slot)) {
		if (!this->can_appoint_idea(slot, idea)) {
			continue;
		}

		const int skill = idea->get_skill();

		if (skill < best_skill) {
			continue;
		}

		if (skill > best_skill) {
			best_skill = skill;
			potential_ideas.clear();
		}

		potential_ideas.push_back(idea);
	}

	if (!potential_ideas.empty()) {
		const idea *idea = vector::get_random(potential_ideas);
		return idea;
	}

	return nullptr;
}

bool domain_game_data::can_have_idea(const idea_slot *slot, const idea *idea) const
{
	if (!idea->is_available_for_country_slot(this->domain, slot)) {
		return false;
	}

	for (const auto &[loop_slot, slot_idea] : this->get_ideas(slot->get_idea_type())) {
		if (slot_idea == idea) {
			return false;
		}
	}

	return true;
}

bool domain_game_data::can_gain_idea(const idea_slot *slot, const idea *idea) const
{
	if (!this->can_have_idea(slot, idea)) {
		return false;
	}

	for (const auto &[loop_slot, slot_idea] : this->get_appointed_ideas(slot->get_idea_type())) {
		if (slot_idea == idea) {
			return false;
		}
	}

	return true;
}

bool domain_game_data::can_appoint_idea(const idea_slot *slot, const idea *idea) const
{
	if (!this->can_gain_idea(slot, idea)) {
		return false;
	}

	const commodity_map<int> commodity_costs = this->get_idea_commodity_costs(idea);
	for (const auto &[commodity, cost] : commodity_costs) {
		if (this->get_economy()->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

std::vector<const idea_slot *> domain_game_data::get_available_idea_slots(const idea_type idea_type) const
{
	std::vector<const idea_slot *> available_idea_slots;

	switch (idea_type) {
		case idea_type::deity:
			for (const deity_slot *slot : deity_slot::get_all()) {
				available_idea_slots.push_back(slot);
			}
			break;
		default:
			assert_throw(false);
	}

	std::erase_if(available_idea_slots, [this](const idea_slot *slot) {
		if (slot->get_conditions() != nullptr && !slot->get_conditions()->check(this->domain, read_only_context(this->domain))) {
			return true;
		}

		return false;
	});

	return available_idea_slots;
}

QVariantList domain_game_data::get_available_deity_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_idea_slots(idea_type::deity));
}

int domain_game_data::get_deity_cost() const
{
	const data_entry_map<idea_slot, const idea *> &deities = this->get_ideas(idea_type::deity);
	const data_entry_map<idea_slot, const idea *> &appointed_deities = this->get_appointed_ideas(idea_type::deity);
	const int deity_count = static_cast<int>(deities.size() + appointed_deities.size());

	if (deity_count == 0) {
		return domain_game_data::first_deity_cost;
	}

	return domain_game_data::base_deity_cost + domain_game_data::deity_cost_increment * (deity_count - 1) * deity_count / 2;
}

commodity_map<int> domain_game_data::get_idea_commodity_costs(const idea *idea) const
{
	commodity_map<int> commodity_costs;

	if (idea->get_idea_type() == idea_type::deity) {
		const int cost = this->get_deity_cost();
		commodity_costs[defines::get()->get_piety_commodity()] = cost;
	}

	return commodity_costs;
}

QVariantList domain_game_data::get_idea_commodity_costs_qvariant_list(const idea *idea) const
{
	return archimedes::map::to_qvariant_list(this->get_idea_commodity_costs(idea));
}

QVariantList domain_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool domain_game_data::has_scripted_modifier(const scripted_domain_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

QCoro::Task<void> domain_game_data::add_scripted_modifier(const scripted_domain_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->domain);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		co_await this->apply_modifier(modifier->get_modifier(), 1);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

QCoro::Task<void> domain_game_data::remove_scripted_modifier(const scripted_domain_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		co_await this->apply_modifier(modifier->get_modifier(), -1);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

QCoro::Task<void> domain_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_domain_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_domain_modifier *modifier : modifiers_to_remove) {
		co_await this->remove_scripted_modifier(modifier);
	}

	//decrement opinion modifiers
	domain_map<std::vector<const opinion_modifier *>> opinion_modifiers_to_remove;

	for (auto &[country, opinion_modifier_map] : this->opinion_modifiers) {
		for (auto &[modifier, duration] : opinion_modifier_map) {
			if (duration == -1) {
				//eternal
				continue;
			}

			--duration;

			if (duration == 0) {
				opinion_modifiers_to_remove[country].push_back(modifier);
			}
		}
	}

	for (const auto &[country, opinion_modifiers] : opinion_modifiers_to_remove) {
		for (const opinion_modifier *modifier : opinion_modifiers) {
			this->remove_opinion_modifier(country, modifier);
		}
	}
}

QCoro::Task<void> domain_game_data::apply_modifier(const modifier<const metternich::domain> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	co_await modifier->apply(this->domain, multiplier);
}

const std::vector<const character *> &domain_game_data::get_characters() const
{
	return this->characters;
}

QVariantList domain_game_data::get_characters_qvariant_list() const
{
	return container::to_qvariant_list(this->get_characters());
}

void domain_game_data::add_character(const character *character)
{
	assert_throw(character->get_game_data()->get_domain() == this->domain);
	assert_throw(!vector::contains(this->get_characters(), character));
	this->characters.push_back(character);

	if (game::get()->is_running()) {
		emit characters_changed();
	}
}

void domain_game_data::remove_character(const character *character)
{
	assert_throw(character->get_game_data()->get_domain() == this->domain);
	assert_throw(vector::contains(this->get_characters(), character));
	std::erase(this->characters, character);

	if (game::get()->is_running()) {
		emit characters_changed();
	}
}

QCoro::Task<void> domain_game_data::check_characters()
{
	const QDate &current_date = game::get()->get_next_date();

	for (const site *site : this->get_sites()) {
		for (const character *site_character : site->get_game_data()->get_homed_characters()) {
			if (site_character->get_game_data()->get_start_date() > current_date) {
				continue;
			}

			if (!site_character->is_immortal() && site_character->get_game_data()->get_death_date() <= current_date) {
				continue;
			}

			if (site_character->get_game_data()->is_dead()) {
				continue;
			}

			if (site_character->get_game_data()->get_domain() != nullptr) {
				continue;
			}

			const metternich::population *home_province_population = site->get_map_data()->get_province()->get_game_data()->get_population();

			//the character's culture must be present in their home province's population
			if (!home_province_population->get_culture_sizes().contains(site_character->get_culture())) {
				continue;
			}

			//the character's religion must be present in their home province's population
			if (!home_province_population->get_religion_sizes().contains(site_character->get_religion())) {
				continue;
			}

			site_character->get_game_data()->set_domain(this->domain);

			if (this->domain == game::get()->get_player_country()) {
				engine_interface::get()->add_notification(std::format("{} Joined Us", site_character->get_game_data()->get_full_name()), site_character, std::format("{} has joined our domain!", site_character->get_game_data()->get_full_name()));
			}

			co_await on_character_recruited(site_character);
		}
	}

	for (const character *character : this->get_characters()) {
		character_game_data *character_game_data = character->get_game_data();

		//recover health and mana
		assert_throw(character_game_data->get_health() > 0);
		co_await character_game_data->fully_recover();

		//check if the portrait is still valid, or should change (e.g. due to aging)
		character_game_data->check_portrait();
	}

	const std::vector<const character *> characters = this->get_characters();
	for (const character *character : characters) {
		if (character->is_immortal()) {
			continue;
		}

		assert_throw(character->get_game_data()->get_death_date().isValid());

		if (character->get_game_data()->get_death_date() > current_date) {
			continue;
		}

		co_await character->get_game_data()->die();
	}

	co_await this->get_government()->check_office_holders();
}

QCoro::Task<void> domain_game_data::on_character_recruited(const character *character)
{
	context ctx(this->domain);
	ctx.source_scope = character;
	co_await domain_event::check_events_for_scope(this->domain, event_trigger::character_recruited, ctx);
}

QCoro::Task<void> domain_game_data::generate_ruler()
{
	const metternich::government_type *government_type = this->get_government_type();
	assert_throw(government_type != nullptr);

	std::vector<const species *> species_list = this->get_culture()->get_species();
	assert_throw(!species_list.empty());

	const species *species = nullptr;
	std::vector<const character_class *> potential_classes;

	while (potential_classes.empty() && !species_list.empty()) {
		species = vector::take_random(species_list);

		for (const character_class *character_class : government_type->get_ruler_character_classes()) {
			if (!character_class->is_allowed_for_species(species)) {
				continue;
			}

			potential_classes.push_back(character_class);
		}
	}

	assert_throw(!potential_classes.empty());

	const character_class *character_class = vector::get_random(potential_classes);

	const character *ruler = co_await character::generate(species, character_class, 1, nullptr, this->get_culture(), this->get_religion(), this->get_capital(), {}, 0, {}, false, gender::none);
	ruler->get_game_data()->set_domain(this->domain);
	co_await this->on_character_recruited(ruler);
	co_await this->get_government()->set_office_holder(defines::get()->get_ruler_office(), ruler);
}

QVariantList domain_game_data::get_historical_rulers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_historical_rulers());
}

void domain_game_data::add_historical_ruler(const character *character)
{
	assert_throw(character != nullptr);

	const QDate current_date = game::get()->get_date();
	this->historical_rulers[current_date] = character;
	character->get_game_data()->add_ruled_domain(this->domain);

	if (this->get_government_type() != nullptr && this->get_government_type()->has_regnal_numbering()) {
		this->historical_monarchs[current_date] = character;
		character->get_game_data()->add_reigned_domain(this->domain);
	}
}

QDate domain_game_data::get_historical_ruler_start_date(const character *character) const
{
	for (const auto &[date, historical_ruler] : this->historical_rulers) {
		if (historical_ruler == character) {
			return date;
		}
	}

	assert_throw(false);
	return QDate();
}

QDate domain_game_data::get_historical_ruler_end_date(const character *character) const
{
	QDate end_date;
	for (auto it = this->historical_rulers.rbegin(); it != this->historical_rulers.rend(); ++it) {
		const auto &[date, historical_ruler] = *it;

		if (historical_ruler == character) {
			return end_date;
		}


		end_date = date;
	}

	return QDate();
}

bool domain_game_data::create_civilian_unit(const civilian_unit_type *civilian_unit_type, const province *deployment_province, const phenotype *phenotype)
{
	if (this->is_under_anarchy()) {
		return false;
	}

	if (deployment_province == nullptr) {
		deployment_province = this->get_capital_province();
	}

	assert_throw(deployment_province != nullptr);

	qunique_ptr<civilian_unit> civilian_unit;

	if (phenotype == nullptr) {
		const phenotype_map<int64_t> phenotype_weights = this->get_phenotype_weights();
		if (phenotype_weights.empty()) {
			return false;
		}

		phenotype = archimedes::map::get_random_weight_map_key(phenotype_weights);
	}

	assert_throw(phenotype != nullptr);

	civilian_unit = make_qunique<metternich::civilian_unit>(civilian_unit_type, this->domain, phenotype);

	assert_throw(civilian_unit != nullptr);

	civilian_unit->set_province(deployment_province);

	this->add_civilian_unit(std::move(civilian_unit));

	return true;
}

void domain_game_data::add_civilian_unit(qunique_ptr<civilian_unit> &&civilian_unit)
{
	if (this->domain == game::get()->get_player_country()) {
		engine_interface::get()->add_active_civilian_unit(civilian_unit.get());
	}

	this->add_unit_name(civilian_unit->get_name());
	this->civilian_units.push_back(std::move(civilian_unit));
}

void domain_game_data::remove_civilian_unit(civilian_unit *civilian_unit)
{
	assert_throw(civilian_unit != nullptr);

	if (this->domain == game::get()->get_player_country()) {
		engine_interface::get()->remove_active_civilian_unit(civilian_unit);
	}

	this->remove_unit_name(civilian_unit->get_name());

	for (size_t i = 0; i < this->civilian_units.size(); ++i) {
		if (this->civilian_units[i].get() == civilian_unit) {
			this->civilian_units.erase(this->civilian_units.begin() + i);
			return;
		}
	}
}

bool domain_game_data::can_gain_civilian_unit(const civilian_unit_type *civilian_unit_type) const
{
	if (civilian_unit_type->get_required_technology() != nullptr && !this->get_technology()->has_technology(civilian_unit_type->get_required_technology())) {
		return false;
	}

	if (this->get_culture()->get_civilian_class_unit_type(civilian_unit_type->get_unit_class()) != civilian_unit_type) {
		return false;
	}

	//FIXME: check whether the country has a building capable of training the civilian unit type

	return true;
}

void domain_game_data::change_civilian_unit_recruitment_count(const civilian_unit_type *civilian_unit_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->civilian_unit_recruitment_counts[civilian_unit_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->civilian_unit_recruitment_counts.erase(civilian_unit_type);
	}

	if (change_input_storage) {
		const commodity_map<int64_t> &commodity_costs = civilian_unit_type->get_commodity_costs();
		for (const auto &[commodity, cost] : commodity_costs) {
			assert_throw(commodity->is_storable());

			const int64_t cost_change = cost * change;

			this->get_economy()->change_stored_commodity(commodity, -cost_change);
		}
	}
}

bool domain_game_data::can_increase_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type) const
{
	if (!this->can_gain_civilian_unit(civilian_unit_type)) {
		return false;
	}

	for (const auto &[commodity, cost] : civilian_unit_type->get_commodity_costs()) {
		assert_throw(commodity->is_storable());
		if (this->get_economy()->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

void domain_game_data::increase_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type)
{
	try {
		assert_throw(this->can_increase_civilian_unit_recruitment(civilian_unit_type));

		this->change_civilian_unit_recruitment_count(civilian_unit_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" civilian unit type for country \"{}\".", civilian_unit_type->get_identifier(), this->domain->get_identifier())));
	}
}

bool domain_game_data::can_decrease_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type) const
{
	if (this->get_civilian_unit_recruitment_count(civilian_unit_type) == 0) {
		return false;
	}

	return true;
}

void domain_game_data::decrease_civilian_unit_recruitment(const civilian_unit_type *civilian_unit_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_civilian_unit_recruitment(civilian_unit_type));

		this->change_civilian_unit_recruitment_count(civilian_unit_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" civilian unit type for country \"{}\".", civilian_unit_type->get_identifier(), this->domain->get_identifier())));
	}
}

QVariantList domain_game_data::get_transporters_qvariant_list() const
{
	return container::to_qvariant_list(this->transporters);
}

bool domain_game_data::create_transporter(const transporter_type *transporter_type, const phenotype *phenotype)
{
	assert_throw(transporter_type != nullptr);

	if (this->is_under_anarchy()) {
		return false;
	}

	qunique_ptr<transporter> transporter;

	if (phenotype == nullptr) {
		const phenotype_map<int64_t> phenotype_weights = this->get_phenotype_weights();
		assert_throw(!phenotype_weights.empty());
		phenotype = archimedes::map::get_random_weight_map_key(phenotype_weights);
	}
	assert_throw(phenotype != nullptr);

	transporter = make_qunique<metternich::transporter>(transporter_type, this->domain, phenotype);

	assert_throw(transporter != nullptr);

	this->add_transporter(std::move(transporter));

	return true;
}

void domain_game_data::add_transporter(qunique_ptr<transporter> &&transporter)
{
	this->add_unit_name(transporter->get_name());
	this->transporters.push_back(std::move(transporter));

	emit transporters_changed();
}

void domain_game_data::remove_transporter(transporter *transporter)
{
	this->remove_unit_name(transporter->get_name());

	for (size_t i = 0; i < this->transporters.size(); ++i) {
		if (this->transporters[i].get() == transporter) {
			this->transporters.erase(this->transporters.begin() + i);
			return;
		}
	}

	emit transporters_changed();
}

void domain_game_data::change_transporter_recruitment_count(const transporter_type *transporter_type, const int change, const bool change_input_storage)
{
	if (change == 0) {
		return;
	}

	const int count = (this->transporter_recruitment_counts[transporter_type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->transporter_recruitment_counts.erase(transporter_type);
	}

	if (change_input_storage) {
		const int old_count = count - change;
		const commodity_map<int64_t> old_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, old_count);
		const commodity_map<int64_t> new_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, count);

		for (const auto &[commodity, cost] : new_commodity_costs) {
			assert_throw(commodity->is_storable());

			const int64_t cost_change = cost - old_commodity_costs.find(commodity)->second;

			this->get_economy()->change_stored_commodity(commodity, -cost_change);
		}

		if (transporter_type->get_wealth_cost() > 0) {
			const int wealth_cost_change = this->get_transporter_type_wealth_cost(transporter_type, count) - this->get_transporter_type_wealth_cost(transporter_type, old_count);
			this->get_economy()->change_wealth(-wealth_cost_change);
		}
	}
}

bool domain_game_data::can_increase_transporter_recruitment(const transporter_type *transporter_type) const
{
	if (this->get_best_transporter_category_type(transporter_type->get_category()) != transporter_type) {
		return false;
	}

	const int old_count = this->get_transporter_recruitment_count(transporter_type);
	const int new_count = old_count + 1;
	const commodity_map<int64_t> old_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, old_count);
	const commodity_map<int64_t> new_commodity_costs = this->get_transporter_type_commodity_costs(transporter_type, new_count);

	for (const auto &[commodity, cost] : new_commodity_costs) {
		assert_throw(commodity->is_storable());

		const int64_t cost_change = cost - old_commodity_costs.find(commodity)->second;

		if (this->get_economy()->get_stored_commodity(commodity) < cost_change) {
			return false;
		}
	}

	if (transporter_type->get_wealth_cost() > 0) {
		const int wealth_cost_change = this->get_transporter_type_wealth_cost(transporter_type, new_count) - this->get_transporter_type_wealth_cost(transporter_type, old_count);

		if (this->get_economy()->get_wealth() < wealth_cost_change) {
			return false;
		}
	}

	return true;
}

void domain_game_data::increase_transporter_recruitment(const transporter_type *transporter_type)
{
	try {
		assert_throw(this->can_increase_transporter_recruitment(transporter_type));

		this->change_transporter_recruitment_count(transporter_type, 1);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error increasing recruitment of the \"{}\" transporter type for country \"{}\".", transporter_type->get_identifier(), this->domain->get_identifier())));
	}
}

bool domain_game_data::can_decrease_transporter_recruitment(const transporter_type *transporter_type) const
{
	if (this->get_transporter_recruitment_count(transporter_type) == 0) {
		return false;
	}

	return true;
}

void domain_game_data::decrease_transporter_recruitment(const transporter_type *transporter_type, const bool restore_inputs)
{
	try {
		assert_throw(this->can_decrease_transporter_recruitment(transporter_type));

		this->change_transporter_recruitment_count(transporter_type, -1, restore_inputs);
	} catch (...) {
		std::throw_with_nested(std::runtime_error(std::format("Error decreasing recruitment of the \"{}\" transporter type for country \"{}\".", transporter_type->get_identifier(), this->domain->get_identifier())));
	}
}

int domain_game_data::get_transporter_type_cost_modifier(const transporter_type *transporter_type) const
{
	//FIXME: implement cost modifiers for transporters

	Q_UNUSED(transporter_type);

	return 0;
}

int domain_game_data::get_transporter_type_wealth_cost(const transporter_type *transporter_type, const int quantity) const
{
	int wealth_cost = transporter_type->get_wealth_cost() * quantity;

	const int cost_modifier = this->get_transporter_type_cost_modifier(transporter_type);
	wealth_cost *= 100 + cost_modifier;
	wealth_cost /= 100;

	if (transporter_type->get_wealth_cost() > 0 && quantity > 0) {
		wealth_cost = std::max(wealth_cost, 1);
	}

	return wealth_cost;
}

commodity_map<int64_t> domain_game_data::get_transporter_type_commodity_costs(const transporter_type *transporter_type, const int quantity) const
{
	commodity_map<int64_t> commodity_costs = transporter_type->get_commodity_costs();

	for (auto &[commodity, cost_int] : commodity_costs) {
		assert_throw(commodity->is_storable());

		centesimal_int cost(cost_int);
		cost *= quantity;

		const int cost_modifier = this->get_transporter_type_cost_modifier(transporter_type);
		cost *= 100 + cost_modifier;
		cost /= 100;

		cost_int = cost.to_int64();

		if (cost_modifier < 0 && cost.get_fractional_value() > 0) {
			cost_int += 1;
		}

		if (quantity > 0) {
			cost_int = std::max(cost_int, 1ll);
		}
	}

	return commodity_costs;
}

QVariantList domain_game_data::get_transporter_type_commodity_costs_qvariant_list(const transporter_type *transporter_type, const int quantity) const
{
	return archimedes::map::to_qvariant_list(this->get_transporter_type_commodity_costs(transporter_type, quantity));
}

const transporter_type *domain_game_data::get_best_transporter_category_type(const transporter_category category, const metternich::culture *culture) const
{
	const transporter_type *best_type = nullptr;
	int best_score = -1;

	for (const transporter_class *transporter_class : transporter_class::get_all()) {
		if (transporter_class->get_category() != category) {
			continue;
		}

		const transporter_type *type = culture->get_transporter_class_type(transporter_class);

		if (type == nullptr) {
			continue;
		}

		if (type->get_required_technology() != nullptr && !this->get_technology()->has_technology(type->get_required_technology())) {
			continue;
		}

		bool upgrade_is_available = false;
		for (const transporter_type *upgrade : type->get_upgrades()) {
			if (culture->get_transporter_class_type(upgrade->get_transporter_class()) != upgrade) {
				continue;
			}

			if (upgrade->get_required_technology() != nullptr && !this->get_technology()->has_technology(upgrade->get_required_technology())) {
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

const transporter_type *domain_game_data::get_best_transporter_category_type(const transporter_category category) const
{
	return this->get_best_transporter_category_type(category, this->get_culture());
}

void domain_game_data::set_transporter_type_stat_modifier(const transporter_type *type, const transporter_stat stat, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_transporter_type_stat_modifier(type, stat);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->transporter_type_stat_modifiers[type].erase(stat);

		if (this->transporter_type_stat_modifiers[type].empty()) {
			this->transporter_type_stat_modifiers.erase(type);
		}
	} else {
		this->transporter_type_stat_modifiers[type][stat] = value;
	}

	const centesimal_int difference = value - old_value;
	for (const qunique_ptr<transporter> &transporter : this->transporters) {
		if (transporter->get_type() != type) {
			continue;
		}

		transporter->change_stat(stat, difference);
	}
}

void domain_game_data::set_population_type_modifier_multiplier(const population_type *type, const centesimal_int &value)
{
	const centesimal_int old_value = this->get_population_type_modifier_multiplier(type);

	if (value == old_value) {
		return;
	}

	assert_throw(type->get_domain_modifier() != nullptr);

	if (value == 1) {
		this->population_type_modifier_multipliers.erase(type);
	} else {
		this->population_type_modifier_multipliers[type] = value;
	}

	//const int population_type_count = this->get_population()->get_type_count(type);
	//const centesimal_int &max_modifier_multiplier = type->get_max_modifier_multiplier();

	//type->get_country_modifier()->apply(this->domain, -centesimal_int::min(population_type_count * old_value, max_modifier_multiplier));
	//type->get_country_modifier()->apply(this->domain, centesimal_int::min(population_type_count * value, max_modifier_multiplier));
}

void domain_game_data::set_building_class_cost_efficiency_modifier(const building_class *building_class, const int value)
{
	if (value == this->get_building_class_cost_efficiency_modifier(building_class)) {
		return;
	}

	if (value == 0) {
		this->building_class_cost_efficiency_modifiers.erase(building_class);
	} else {
		this->building_class_cost_efficiency_modifiers[building_class] = value;
	}
}

bool domain_game_data::is_tile_explored(const QPoint &tile_pos) const
{
	if constexpr (map::exploration_enabled) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->get_province() != nullptr && this->explored_provinces.contains(tile->get_province())) {
			return true;
		}

		return false;
	} else {
		//exploration mechanics are disabled, the entire map is always explored
		return true;
	}
}

bool domain_game_data::is_province_discovered(const province *province) const
{
	//get whether the province has been at least partially explored
	const province_map_data *province_map_data = province->get_map_data();
	if (!province_map_data->is_on_map()) {
		return false;
	}

	if (this->is_province_explored(province)) {
		return true;
	}

	return false;
}

bool domain_game_data::is_province_explored(const metternich::province *province) const
{
	if constexpr (map::exploration_enabled) {
		//get whether the province has been fully explored
		return this->explored_provinces.contains(province);
	} else {
		return true;
	}
}

bool domain_game_data::is_region_discovered(const region *region) const
{
	for (const province *province : region->get_provinces()) {
		const province_game_data *province_game_data = province->get_game_data();
		if (!province_game_data->is_on_map()) {
			continue;
		}

		if (this->is_province_explored(province)) {
			return true;
		}
	}

	return false;
}

QCoro::Task<void> domain_game_data::explore_province(const province *province)
{
	if (this->explored_provinces.contains(province)) {
		co_return;
	}

	const province_map_data *province_map_data = province->get_map_data();
	assert_throw(province_map_data->is_on_map());

	const province_game_data *province_game_data = province->get_game_data();
	const metternich::domain *province_owner = province_game_data->get_owner();

	if (province_owner != nullptr && province_owner != this->domain && !this->is_country_known(province_owner)) {
		co_await this->add_known_country(province_owner);
	}

	for (const site *site : province_game_data->get_sites()) {
		const metternich::domain *site_owner = site->get_game_data()->get_owner();

		if (site_owner != nullptr && site_owner != this->domain && !this->is_country_known(site_owner)) {
			co_await this->add_known_country(site_owner);
		}
	}


	this->explored_provinces.insert(province);

	if (this->domain == game::get()->get_player_country()) {
		map::get()->update_minimap_rect(province_map_data->get_territory_rect());
		game::get()->set_exploration_changed();
	}

	for (const QPoint &tile_pos : province_map_data->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const resource *tile_resource = tile->get_resource();

		if (tile_resource != nullptr && tile->is_resource_discovered() && tile_resource->get_discovery_technology() != nullptr) {
			if (this->get_technology()->can_gain_technology(tile_resource->get_discovery_technology())) {
				co_await this->get_technology()->add_technology(tile_resource->get_discovery_technology());

				if (game::get()->is_running()) {
					emit get_technology()->technology_researched(tile_resource->get_discovery_technology());
				}
			}
		}
	}
}

QCoro::Task<void> domain_game_data::prospect_tile(const QPoint &tile_pos)
{
	this->prospected_tiles.insert(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);
	const resource *tile_resource = tile->get_resource();

	if (tile_resource != nullptr) {
		if (!tile->is_resource_discovered() && (tile_resource->get_required_technology() == nullptr || this->get_technology()->has_technology(tile_resource->get_required_technology()))) {
			co_await map::get()->set_tile_resource_discovered(tile_pos, true);
		}
	}

	emit prospected_tiles_changed();

	if (this->domain == game::get()->get_player_country()) {
		emit map::get()->tile_prospection_changed(tile_pos);
	}
}

void domain_game_data::reset_tile_prospection(const QPoint &tile_pos)
{
	this->prospected_tiles.erase(tile_pos);

	emit prospected_tiles_changed();

	if (this->domain == game::get()->get_player_country()) {
		emit map::get()->tile_prospection_changed(tile_pos);
	}
}

QVariantList domain_game_data::get_active_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_active_journal_entries());
}

QCoro::Task<void> domain_game_data::add_active_journal_entry(const journal_entry *journal_entry)
{
	this->active_journal_entries.push_back(journal_entry);

	if (journal_entry->get_active_modifier() != nullptr) {
		co_await journal_entry->get_active_modifier()->apply(this->domain);
	}

	for (const building_type *building : journal_entry->get_built_buildings_with_requirements()) {
		this->get_ai()->change_building_desire_modifier(building, journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->get_ai()->change_settlement_building_desire_modifier(settlement, building, journal_entry::ai_building_desire_modifier);
		}
	}
}

QCoro::Task<void> domain_game_data::remove_active_journal_entry(const journal_entry *journal_entry)
{
	std::erase(this->active_journal_entries, journal_entry);

	if (journal_entry->get_active_modifier() != nullptr) {
		co_await journal_entry->get_active_modifier()->remove(this->domain);
	}

	for (const building_type *building : journal_entry->get_built_buildings_with_requirements()) {
		this->get_ai()->change_building_desire_modifier(building, -journal_entry::ai_building_desire_modifier);
	}

	for (const auto &[settlement, buildings] : journal_entry->get_built_settlement_buildings_with_requirements()) {
		for (const building_type *building : buildings) {
			this->get_ai()->change_settlement_building_desire_modifier(settlement, building, -journal_entry::ai_building_desire_modifier);
		}
	}
}

QVariantList domain_game_data::get_inactive_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_inactive_journal_entries());
}

QVariantList domain_game_data::get_finished_journal_entries_qvariant_list() const
{
	return container::to_qvariant_list(this->get_finished_journal_entries());
}

QCoro::Task<void> domain_game_data::check_journal_entries(const bool ignore_effects, const bool ignore_random_chance)
{
	const read_only_context ctx(this->domain);

	bool changed = false;

	//check if any journal entry has become potentially available
	if (this->check_potential_journal_entries()) {
		changed = true;
	}

	if (co_await this->check_inactive_journal_entries()) {
		changed = true;
	}

	if (co_await this->check_active_journal_entries(ctx, ignore_effects, ignore_random_chance)) {
		changed = true;
	}

	if (changed) {
		emit journal_entries_changed();
	}
}

bool domain_game_data::check_potential_journal_entries()
{
	bool changed = false;

	for (const journal_entry *journal_entry : journal_entry::get_all()) {
		if (vector::contains(this->get_active_journal_entries(), journal_entry)) {
			continue;
		}

		if (vector::contains(this->get_inactive_journal_entries(), journal_entry)) {
			continue;
		}

		if (vector::contains(this->get_finished_journal_entries(), journal_entry)) {
			continue;
		}

		if (!journal_entry->check_preconditions(this->domain)) {
			continue;
		}

		this->inactive_journal_entries.push_back(journal_entry);
		changed = true;
	}

	return changed;
}

QCoro::Task<bool> domain_game_data::check_inactive_journal_entries()
{
	bool changed = false;

	const std::vector<const journal_entry *> inactive_entries = this->get_inactive_journal_entries();

	for (const journal_entry *journal_entry : inactive_entries) {
		if (!journal_entry->check_preconditions(this->domain)) {
			std::erase(this->inactive_journal_entries, journal_entry);
			changed = true;
			continue;
		}

		if (!journal_entry->check_conditions(this->domain)) {
			continue;
		}

		std::erase(this->inactive_journal_entries, journal_entry);

		co_await this->add_active_journal_entry(journal_entry);

		changed = true;
	}

	co_return changed;
}

QCoro::Task<bool> domain_game_data::check_active_journal_entries(const read_only_context &ctx, const bool ignore_effects, const bool ignore_random_chance)
{
	bool changed = false;

	const std::vector<const journal_entry *> active_entries = this->get_active_journal_entries();

	for (const journal_entry *journal_entry : active_entries) {
		if (!journal_entry->check_preconditions(this->domain)) {
			co_await this->remove_active_journal_entry(journal_entry);
			changed = true;
			continue;
		}

		if (!journal_entry->check_conditions(this->domain)) {
			co_await this->remove_active_journal_entry(journal_entry);
			this->inactive_journal_entries.push_back(journal_entry);
			changed = true;
			continue;
		}

		if (journal_entry->check_completion_conditions(this->domain, ignore_random_chance)) {
			co_await this->remove_active_journal_entry(journal_entry);
			this->finished_journal_entries.push_back(journal_entry);
			if (!ignore_effects) {
				if (journal_entry->get_completion_effects() != nullptr) {
					context effects_ctx(this->domain);
					co_await journal_entry->get_completion_effects()->do_effects(this->domain, effects_ctx);
				}

				if (game::get()->is_running()) {
					emit journal_entry_completed(journal_entry);
				}
			}

			if (journal_entry->get_completion_modifier() != nullptr) {
				co_await journal_entry->get_completion_modifier()->apply(this->domain);
			}

			changed = true;
		} else if (journal_entry->get_failure_conditions() != nullptr && journal_entry->get_failure_conditions()->check(this->domain, ctx)) {
			co_await this->remove_active_journal_entry(journal_entry);
			this->finished_journal_entries.push_back(journal_entry);
			if (journal_entry->get_failure_effects() != nullptr && !ignore_effects) {
				context effects_ctx(this->domain);
				co_await journal_entry->get_failure_effects()->do_effects(this->domain, effects_ctx);

				if (this->domain == game::get()->get_player_country()) {
					engine_interface::get()->add_notification(journal_entry->get_name(), journal_entry->get_portrait(), std::format("{}{}{}", journal_entry->get_description(), (!journal_entry->get_description().empty() ? "\n\n" : ""), journal_entry->get_failure_effects()->get_effects_string(this->domain, ctx)));
				}
			}
			changed = true;
		}
	}

	co_return changed;
}

QCoro::Task<void> domain_game_data::set_free_building_class_count(const building_class *building_class, const int value)
{
	const int old_value = this->get_free_building_class_count(building_class);
	if (value == old_value) {
		co_return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_building_class_counts.erase(building_class);
	} else if (old_value == 0) {
		this->free_building_class_counts[building_class] = value;

		for (const site *site : this->get_sites()) {
			if (!site->is_settlement() || !site->get_game_data()->is_built()) {
				continue;
			}

			co_await site->get_game_data()->check_free_buildings();
		}
	}
}

void domain_game_data::set_free_consulate_count(const consulate *consulate, const int value)
{
	const int old_value = this->get_free_consulate_count(consulate);
	if (value == old_value) {
		return;
	}

	assert_throw(value >= 0);

	if (value == 0) {
		this->free_consulate_counts.erase(consulate);
	} else if (old_value == 0) {
		this->free_consulate_counts[consulate] = value;

		for (const metternich::domain *known_country : this->get_known_countries()) {
			const metternich::consulate *current_consulate = this->get_consulate(known_country);
			if (current_consulate == nullptr || current_consulate->get_level() < consulate->get_level()) {
				this->set_consulate(known_country, consulate);
			}
		}
	}
}

int64_t domain_game_data::get_min_income() const
{
	int64_t min_income = this->get_economy()->get_commodity_output(defines::get()->get_wealth_commodity()).to_int64();

	for (const province *province : this->get_provinces()) {
		min_income += province->get_game_data()->get_min_income();
	}

	for (const site *holding_site : this->get_sites()) {
		if (!holding_site->is_settlement() || !holding_site->get_game_data()->is_built()) {
			continue;
		}

		min_income += holding_site->get_game_data()->get_min_income();
	}

	return min_income;
}

int64_t domain_game_data::get_max_income() const
{
	int64_t max_income = this->get_economy()->get_commodity_output(defines::get()->get_wealth_commodity()).to_int64();

	for (const province *province : this->get_provinces()) {
		max_income += province->get_game_data()->get_max_income();
	}

	for (const site *holding_site : this->get_sites()) {
		if (!holding_site->is_settlement() || !holding_site->get_game_data()->is_built()) {
			continue;
		}

		max_income += holding_site->get_game_data()->get_max_income();
	}

	for (const auto &[attribute, attribute_value] : this->get_attribute_values()) {
		if (!attribute->is_taxable()) {
			continue;
		}

		const int max_result = (20 + attribute_value - this->get_effective_unrest()) / 3;
		max_income += max_result * defines::get()->get_domain_income_unit_value();
	}

	return max_income;
}

int64_t domain_game_data::get_domain_maintenance_cost() const
{
	const int domain_size = this->get_province_count() + this->get_holding_count();
	assert_throw(domain_size > 0);

	return defines::get()->get_domain_maintenance_cost_for_domain_size(domain_size);
}

int64_t domain_game_data::get_maintenance_cost() const
{
	int64_t maintenance_cost = this->get_domain_maintenance_cost();

	for (const qunique_ptr<military_unit> &military_unit : this->get_military()->get_military_units()) {
		const auto find_iterator = military_unit->get_type()->get_maintenance_commodity_costs().find(defines::get()->get_wealth_commodity());
		if (find_iterator != military_unit->get_type()->get_maintenance_commodity_costs().end()) {
			maintenance_cost += find_iterator->second;
		}
	}

	return maintenance_cost;
}

bool domain_game_data::can_visit_site(const metternich::site *site) const
{
	//a site can be visited if either this domain owns the site or its province, or if it owns any site within the province

	const province *site_province = site->get_game_data()->get_province();
	if (site->get_game_data()->get_owner() == this->domain || site_province->get_game_data()->get_owner() == this->domain) {
		return true;
	}

	for (const metternich::site *province_site : site_province->get_game_data()->get_sites()) {
		if (province_site->get_game_data()->get_owner() == this->domain) {
			return true;
		}
	}

	return false;
}

}
