#include "metternich.h"

#include "country/country_government.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "character/character_trait.h"
#include "character/character_trait_type.h"
#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "country/country_technology.h"
#include "country/government_group.h"
#include "country/government_type.h"
#include "country/journal_entry.h"
#include "country/law.h"
#include "country/law_group.h"
#include "country/office.h"
#include "database/defines.h"
#include "engine_interface.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "ui/portrait.h"
#include "unit/civilian_unit.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/map_util.h"
#include "util/string_util.h"
#include "util/vector_util.h"
#include "util/vector_random_util.h"

namespace metternich {

country_government::country_government(const metternich::country *country, const country_game_data *game_data)
	: country(country)
{
	connect(game_data, &country_game_data::tier_changed, this, &country_government::office_title_names_changed);
	connect(this, &country_government::government_type_changed, this, &country_government::office_title_names_changed);
	connect(game_data, &country_game_data::religion_changed, this, &country_government::office_title_names_changed);
	connect(this, &country_government::office_holders_changed, this, &country_government::office_title_names_changed);
}

country_government::~country_government()
{
}

country_game_data *country_government::get_game_data() const
{
	return this->country->get_game_data();
}

const std::string &country_government::get_office_title_name(const office *office) const
{
	const character *office_holder = this->get_office_holder(office);
	const gender gender = office_holder != nullptr ? office_holder->get_gender() : gender::male;
	return this->country->get_office_title_name(office, this->get_government_type(), this->get_game_data()->get_tier(), gender, this->get_game_data()->get_religion());
}

void country_government::set_government_type(const metternich::government_type *government_type)
{
	if (government_type == this->get_government_type()) {
		return;
	}

	if (government_type != nullptr) {
		if (this->country->is_tribe() && !government_type->get_group()->is_tribal()) {
			throw std::runtime_error(std::format("Tried to set a non-tribal government type (\"{}\") for a tribal country (\"{}\").", government_type->get_identifier(), this->country->get_identifier()));
		}

		if (this->country->is_clade() && !government_type->get_group()->is_clade()) {
			throw std::runtime_error(std::format("Tried to set a non-clade government type (\"{}\") for a clade country (\"{}\").", government_type->get_identifier(), this->country->get_identifier()));
		}
	}

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		this->get_government_type()->get_modifier()->apply(this->country, -1);
	}

	this->government_type = government_type;

	if (this->get_government_type() != nullptr && this->get_government_type()->get_modifier() != nullptr) {
		this->get_government_type()->get_modifier()->apply(this->country, 1);
	}

	if (game::get()->is_running()) {
		emit government_type_changed();
	}
}

bool country_government::can_have_government_type(const metternich::government_type *government_type) const
{
	if (government_type->get_required_technology() != nullptr && !this->country->get_technology()->has_technology(government_type->get_required_technology())) {
		return false;
	}

	for (const law *forbidden_law : government_type->get_forbidden_laws()) {
		if (this->has_law(forbidden_law)) {
			return false;
		}
	}

	if (government_type->get_conditions() != nullptr && !government_type->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	return true;
}

void country_government::check_government_type()
{
	if (this->get_government_type() != nullptr && this->can_have_government_type(this->get_government_type())) {
		return;
	}

	std::vector<const metternich::government_type *> potential_government_types;

	for (const metternich::government_type *government_type : government_type::get_all()) {
		if (this->can_have_government_type(government_type)) {
			potential_government_types.push_back(government_type);
		}
	}

	assert_throw(!potential_government_types.empty());

	this->set_government_type(vector::get_random(potential_government_types));
}

bool country_government::is_tribal() const
{
	return this->get_government_type()->get_group()->is_tribal();
}

bool country_government::is_clade() const
{
	return this->get_government_type()->get_group()->is_clade();
}

QVariantList country_government::get_laws_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_laws());
}

void country_government::set_law(const law_group *law_group, const law *law)
{
	assert_throw(law_group != nullptr);

	if (law == this->get_law(law_group)) {
		return;
	}

	const metternich::law *old_law = this->get_law(law_group);
	if (old_law != nullptr) {
		old_law->get_modifier()->remove(this->country);
	}

	this->laws[law_group] = law;

	if (law != nullptr) {
		assert_throw(law->get_group() == law_group);
		law->get_modifier()->apply(this->country);
	}

	this->check_government_type();

	if (game::get()->is_running()) {
		emit laws_changed();
	}
}

bool country_government::has_law(const law *law) const
{
	return this->get_law(law->get_group()) == law;
}

bool country_government::can_have_law(const metternich::law *law) const
{
	if (law->get_required_technology() != nullptr && !this->country->get_technology()->has_technology(law->get_required_technology())) {
		return false;
	}

	if (law->get_conditions() != nullptr && !law->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	return true;
}

bool country_government::can_enact_law(const metternich::law *law) const
{
	if (!this->can_have_law(law)) {
		return false;
	}

	if (vector::contains(this->get_government_type()->get_forbidden_laws(), law)) {
		return false;
	}

	for (const auto &[commodity, cost] : law->get_commodity_costs()) {
		if (this->country->get_economy()->get_stored_commodity(commodity) < (cost * this->get_total_law_cost_modifier() / 100)) {
			return false;
		}
	}

	return true;
}

void country_government::enact_law(const law *law)
{
	for (const auto &[commodity, cost] : law->get_commodity_costs()) {
		this->country->get_economy()->change_stored_commodity(commodity, -cost * this->get_total_law_cost_modifier() / 100);
	}

	this->set_law(law->get_group(), law);
}

int country_government::get_total_law_cost_modifier() const
{
	return 100 + (this->get_game_data()->get_population_unit_count() - 1) + this->get_law_cost_modifier();
}

void country_government::check_laws()
{
	for (const law_group *law_group : law_group::get_all()) {
		if (this->get_law(law_group) != nullptr && !this->can_have_law(this->get_law(law_group))) {
			this->set_law(law_group, nullptr);
		}

		if (this->get_law(law_group) == nullptr) {
			const law *government_type_default_law = this->get_government_type()->get_default_law(law_group);
			if (government_type_default_law != nullptr && this->can_have_law(government_type_default_law)) {
				this->set_law(law_group, government_type_default_law);
			}
		}

		if (this->get_law(law_group) == nullptr) {
			if (this->can_have_law(law_group->get_default_law())) {
				this->set_law(law_group, law_group->get_default_law());
			}
		}

		if (this->get_law(law_group) == nullptr) {
			std::vector<const law *> potential_laws;
			for (const metternich::law *group_law : law_group->get_laws()) {
				if (this->can_have_law(group_law)) {
					potential_laws.push_back(group_law);
				}
			}
			if (!potential_laws.empty()) {
				this->set_law(law_group, vector::get_random(potential_laws));
			}
		}
	}
}

const character *country_government::get_ruler() const
{
	return this->get_office_holder(defines::get()->get_ruler_office());
}

QVariantList country_government::get_office_holders_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_office_holders());
}

void country_government::set_office_holder(const office *office, const character *character)
{
	const metternich::character *old_office_holder = this->get_office_holder(office);

	if (character == old_office_holder) {
		return;
	}

	if (old_office_holder != nullptr) {
		old_office_holder->get_game_data()->apply_office_modifier(this->country, office, -1);
		old_office_holder->get_game_data()->set_office(nullptr);
		old_office_holder->get_game_data()->set_country(nullptr);
	}

	const metternich::office *old_office = character ? character->get_game_data()->get_office() : nullptr;
	if (old_office != nullptr) {
		this->set_office_holder(old_office, nullptr);
	}

	if (character != nullptr) {
		this->office_holders[office] = character;
	} else {
		this->office_holders.erase(office);
	}

	if (character != nullptr) {
		character->get_game_data()->apply_office_modifier(this->country, office, 1);
		character->get_game_data()->set_office(office);
		character->get_game_data()->set_country(this->country);
	}

	if (old_office != nullptr) {
		this->check_office_holder(old_office, character);
	}

	if (office->is_ruler()) {
		if (this->country == game::get()->get_player_country()) {
			game::get()->set_player_character(character);
		}
	}

	if (game::get()->is_running()) {
		emit office_holders_changed();

		if (office->is_ruler()) {
			emit ruler_changed();

			if (old_office_holder != nullptr) {
				emit old_office_holder->get_game_data()->ruler_changed();
			}

			if (character != nullptr) {
				emit character->get_game_data()->ruler_changed();
			}
		}

		if (this->country == game::get()->get_player_country() && character != nullptr) {
			const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

			engine_interface::get()->add_notification(std::format("New {}", office->get_name()), interior_minister_portrait, std::format("{} has become our new {}!\n\n{}", character->get_full_name(), string::lowered(office->get_name()), character->get_game_data()->get_office_modifier_string(this->country, office)));
		}
	}
}

QVariantList country_government::get_appointed_office_holders_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_appointed_office_holders());
}

void country_government::set_appointed_office_holder(const office *office, const character *character)
{
	assert_throw(office->is_appointable());

	const metternich::character *old_appointee = this->get_appointed_office_holder(office);

	if (character == old_appointee) {
		return;
	}

	if (character != nullptr) {
		const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->country->get_economy()->change_stored_commodity(commodity, -cost);
		}

		this->appointed_office_holders[office] = character;
	} else {
		this->appointed_office_holders.erase(office);
	}

	if (old_appointee != nullptr) {
		const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
		for (const auto &[commodity, cost] : commodity_costs) {
			this->country->get_economy()->change_stored_commodity(commodity, cost);
		}
	}

	if (game::get()->is_running()) {
		emit appointed_office_holders_changed();
	}
}

void country_government::check_office_holder(const office *office, const character *previous_holder)
{
	if (this->get_game_data()->is_under_anarchy()) {
		this->set_office_holder(office, nullptr);
		return;
	}

	//process appointment, if any
	const character *appointed_holder = this->get_appointed_office_holder(office);
	if (appointed_holder != nullptr && this->can_have_office_holder(office, appointed_holder)) {
		assert_throw(office->is_appointable());
		this->set_office_holder(office, appointed_holder);
		this->set_appointed_office_holder(office, nullptr);
	}

	//remove office holders if they have become obsolete
	if (this->get_office_holder(office) != nullptr && this->get_office_holder(office)->get_obsolescence_technology() != nullptr && this->country->get_technology()->has_technology(this->get_office_holder(office)->get_obsolescence_technology())) {
		this->get_office_holder(office)->get_game_data()->die();
		return; //the office holder death will already trigger a re-check of the office holder position
	}

	//if the country has no holder for a non-appointable office, see if there is any character who can become the holder
	if (this->get_office_holder(office) == nullptr && !office->is_appointable()) {
		const character *character = this->get_best_office_holder(office, previous_holder);
		if (character != nullptr) {
			this->set_office_holder(office, character);
		} else {
			if (office == defines::get()->get_ruler_office()) {
				this->get_game_data()->generate_ruler();
				assert_throw(this->get_ruler() != nullptr);
			}
		}
	}
}

void country_government::check_office_holders()
{
	const std::vector<const office *> available_offices = this->get_available_offices();
	for (const office *office : available_offices) {
		this->check_office_holder(office, nullptr);
	}

	const data_entry_map<office, const character *> office_holders = this->get_office_holders();
	for (const auto &[office, office_holder] : office_holders) {
		if (!vector::contains(available_offices, office)) {
			this->set_office_holder(office, nullptr);
		}
	}

	this->appointed_office_holders.clear();
}

std::vector<const character *> country_government::get_appointable_office_holders(const office *office) const
{
	assert_throw(this->get_government_type() != nullptr);

	std::vector<const character *> potential_holders;

	for (const character *character : character::get_all()) {
		if (!office->is_ruler()) {
			if (!character->has_role(character_role::advisor)) {
				continue;
			}
		}

		if (!this->can_gain_office_holder(office, character)) {
			continue;
		}

		potential_holders.push_back(character);
	}

	for (const qunique_ptr<character> &character : game::get()->get_generated_characters()) {
		if (!office->is_ruler()) {
			if (!character->has_role(character_role::advisor)) {
				continue;
			}
		}

		if (!this->can_gain_office_holder(office, character.get())) {
			continue;
		}

		potential_holders.push_back(character.get());
	}

	return potential_holders;
}

QVariantList country_government::get_appointable_office_holders_qvariant_list(const office *office) const
{
	return container::to_qvariant_list(this->get_appointable_office_holders(office));
}

const character *country_government::get_best_office_holder(const office *office, const character *previous_holder) const
{
	assert_throw(this->get_government_type() != nullptr);

	std::vector<const character *> potential_holders;
	int best_attribute_value = 0;
	bool found_same_dynasty = false;

	for (const character *character : this->get_appointable_office_holders(office)) {
		if (!this->can_appoint_office_holder(office, character)) {
			continue;
		}

		const character_game_data *character_game_data = character->get_game_data();

		if (office->is_ruler()) {
			if (previous_holder != nullptr && previous_holder->get_dynasty() != nullptr) {
				const bool same_dynasty = character->get_dynasty() == previous_holder->get_dynasty();
				if (same_dynasty && !found_same_dynasty) {
					potential_holders.clear();
					best_attribute_value = 0;
					found_same_dynasty = true;
				} else if (!same_dynasty && found_same_dynasty) {
					continue;
				}
			}
		}

		const int attribute_value = character_game_data->get_attribute_value(office->get_attribute());

		if (attribute_value < best_attribute_value) {
			continue;
		}

		if (attribute_value > best_attribute_value) {
			best_attribute_value = attribute_value;
			potential_holders.clear();
		}

		potential_holders.push_back(character);
	}

	if (!potential_holders.empty()) {
		const character *office_holder = vector::get_random(potential_holders);
		return office_holder;
	}

	return nullptr;
}

bool country_government::can_have_office_holder(const office *office, const character *character) const
{
	const character_game_data *character_game_data = character->get_game_data();
	if (character_game_data->get_country() != nullptr && character_game_data->get_country() != this->country) {
		return false;
	}

	if (character->get_home_settlement() == nullptr || (character_game_data->get_country() != this->country && character->get_home_settlement()->get_game_data()->get_owner() != this->country)) {
		return false;
	}

	if (game::get()->get_date() < character->get_start_date()) {
		return false;
	}

	if (character->get_death_date().isValid() && game::get()->get_date() >= character->get_death_date()) {
		return false;
	}

	if (character_game_data->is_dead()) {
		return false;
	}

	if (character->get_required_technology() != nullptr && !this->country->get_technology()->has_technology(character->get_required_technology())) {
		return false;
	}

	if (character->get_obsolescence_technology() != nullptr && this->country->get_technology()->has_technology(character->get_obsolescence_technology())) {
		return false;
	}

	if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->country, read_only_context(this->country))) {
		return false;
	}

	if (office->is_ruler()) {
		if (character->get_character_class() == nullptr || !vector::contains(this->get_government_type()->get_ruler_character_classes(), character->get_character_class())) {
			return false;
		}
	} else {
		if (character_game_data->get_office() != nullptr) {
			return false;
		}
	}

	if (office->get_holder_conditions() != nullptr && !office->get_holder_conditions()->check(character, read_only_context(character))) {
		return false;
	}

	return true;
}

bool country_government::can_gain_office_holder(const office *office, const character *character) const
{
	if (!this->can_have_office_holder(office, character)) {
		return false;
	}

	for (const auto &[loop_office, office_appointee] : this->get_appointed_office_holders()) {
		if (office_appointee == character) {
			return false;
		}
	}

	return true;
}

bool country_government::can_appoint_office_holder(const office *office, const character *character) const
{
	if (!this->can_gain_office_holder(office, character)) {
		return false;
	}

	const commodity_map<int> commodity_costs = this->get_advisor_commodity_costs(office);
	for (const auto &[commodity, cost] : commodity_costs) {
		if (this->country->get_economy()->get_stored_commodity(commodity) < cost) {
			return false;
		}
	}

	return true;
}

void country_government::on_office_holder_died(const office *office, const character *office_holder)
{
	if (game::get()->is_running()) {
		if (this->country == game::get()->get_player_country()) {
			const portrait *interior_minister_portrait = this->get_interior_minister_portrait();

			if (office->is_ruler()) {
				engine_interface::get()->add_notification(std::format("{} Died", office_holder->get_full_name()), interior_minister_portrait, std::format("Our {}, {}, has died!", string::lowered(office->get_name()), office_holder->get_full_name()));
			} else {
				engine_interface::get()->add_notification(std::format("{} Retired", office_holder->get_full_name()), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, {} {} has decided to retire.", string::lowered(office->get_name()), office_holder->get_full_name()));
			}
		}

		if (office->is_ruler()) {
			context ctx(this->country);
			ctx.source_scope = office_holder;
			country_event::check_events_for_scope(this->country, event_trigger::ruler_death, ctx);
		}
	}

	this->check_office_holder(office, office_holder);
}

std::vector<const office *> country_government::get_available_offices() const
{
	std::vector<const office *> available_offices;

	for (const office *office : office::get_all()) {
		if (office->get_conditions() != nullptr && !office->get_conditions()->check(this->country, read_only_context(this->country))) {
			continue;
		}

		available_offices.push_back(office);
	}

	return available_offices;
}

std::vector<const office *> country_government::get_appointable_available_offices() const
{
	std::vector<const office *> available_offices = this->get_available_offices();
	std::erase_if(available_offices, [](const office *office) {
		return !office->is_appointable();
	});
	return available_offices;
}

QVariantList country_government::get_available_offices_qvariant_list() const
{
	return container::to_qvariant_list(this->get_available_offices());
}

commodity_map<int> country_government::get_advisor_commodity_costs(const office *office) const
{
	commodity_map<int> commodity_costs;

	if (office != nullptr && office->is_appointable()) {
		const int cost = this->get_advisor_cost();
		commodity_costs[defines::get()->get_advisor_commodity()] = cost;
	}

	return commodity_costs;
}

QVariantList country_government::get_advisor_commodity_costs_qvariant_list(const office *office) const
{
	return archimedes::map::to_qvariant_list(this->get_advisor_commodity_costs(office));
}

bool country_government::can_have_appointable_offices() const
{
	return !this->get_appointable_available_offices().empty();
}

const metternich::portrait *country_government::get_interior_minister_portrait() const
{
	const character *office_holder = this->get_office_holder(defines::get()->get_interior_minister_office());
	if (office_holder != nullptr) {
		return office_holder->get_game_data()->get_portrait();
	}

	return defines::get()->get_interior_minister_portrait();
}

const metternich::portrait *country_government::get_war_minister_portrait() const
{
	const character *office_holder = this->get_office_holder(defines::get()->get_war_minister_office());
	if (office_holder != nullptr) {
		return office_holder->get_game_data()->get_portrait();
	}

	return defines::get()->get_war_minister_portrait();
}

}
