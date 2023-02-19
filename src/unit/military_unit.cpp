#include "metternich.h"

#include "unit/military_unit.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "infrastructure/improvement.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

const character *military_unit::get_army_commander(const std::vector<military_unit *> &military_units)
{
	const metternich::character *best_character = nullptr;
	int best_skill = 0;

	for (const military_unit *military_unit : military_units) {
		if (military_unit->get_character() == nullptr) {
			continue;
		}

		const int skill = military_unit->get_character()->get_game_data()->get_primary_attribute_value();
		if (skill > best_skill) {
			best_character = military_unit->get_character();
			best_skill = skill;
		}
	}

	return best_character;
}

const character *military_unit::get_army_commander(const std::vector<const military_unit *> &military_units)
{
	const metternich::character *best_character = nullptr;
	int best_skill = 0;

	for (const military_unit *military_unit : military_units) {
		if (military_unit->get_character() == nullptr) {
			continue;
		}

		const int skill = military_unit->get_character()->get_game_data()->get_primary_attribute_value();
		if (skill > best_skill) {
			best_character = military_unit->get_character();
			best_skill = skill;
		}
	}

	return best_character;
}

int military_unit::get_army_score(const std::vector<military_unit *> &military_units)
{
	int score = 0;

	for (const military_unit *military_unit : military_units) {
		score += military_unit->get_type()->get_score();
	}

	return score;
}

military_unit::military_unit(const military_unit_type *type) : type(type)
{
	assert_throw(this->get_type() != nullptr);

	this->max_hit_points = type->get_hit_points();
	this->set_hit_points(this->get_max_hit_points());
	this->set_morale(this->get_hit_points());
}

military_unit::military_unit(const military_unit_type *type, const country *owner, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype)
	: military_unit(type)
{
	this->owner = owner;
	this->population_type = population_type;
	this->culture = culture;
	this->religion = religion;
	this->phenotype = phenotype;

	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	connect(this, &military_unit::type_changed, this, &military_unit::icon_changed);
}

military_unit::military_unit(const military_unit_type *type, const country *owner, const metternich::province *home_province, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype)
	: military_unit(type, owner, culture, religion, phenotype)
{
	this->home_province = home_province;
	this->population_type = population_type;

	assert_throw(this->get_home_province() != nullptr);
	assert_throw(this->get_population_type() != nullptr);

	this->get_home_province()->get_game_data()->add_home_military_unit(this);
}

military_unit::military_unit(const military_unit_type *type, const metternich::character *character)
	: military_unit(type, character->get_game_data()->get_employer(), character->get_culture(), character->get_religion(), character->get_phenotype())
{
	this->character = character;

	//set the hit points here separately, since it could be different after the character is set, as vitality increases unit HP
	this->max_hit_points += (character->get_game_data()->get_vitality() - character::base_vitality) * military_unit::vitality_hit_point_bonus;
	this->set_hit_points(this->get_max_hit_points());
	this->set_morale(this->get_hit_points());

	//character military units do not have any province set as their home province, since they don't consume food
}

void military_unit::do_turn()
{
	if (!this->is_moving()) {
		const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();
		assert_throw(missing_hit_points >= 0);
		if (missing_hit_points > 0) {
			//recover unit HP if it is not moving
			this->change_hit_points(std::min(military_unit::hit_point_recovery_per_turn, missing_hit_points));
		}

		const int missing_morale = this->get_hit_points() - this->get_morale();
		assert_throw(missing_morale >= 0);
		if (missing_morale > 0) {
			this->change_morale(std::min(military_unit::morale_recovery_per_turn, missing_morale));
		}
	}

	if (this->is_moving() && this->get_site() == nullptr) {
		this->set_original_province(nullptr);

		if (this->get_province() != nullptr) {
			this->get_province()->get_game_data()->add_military_unit(this);
		}
	}
}

void military_unit::do_ai_turn()
{
	if (this->is_moving()) {
		return;
	}

	//FIXME: implement logic for upgrading military units, and for moving them to places in order to do combat or defend against attacks
}

void military_unit::set_type(const military_unit_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	const military_unit_type *old_type = this->get_type();

	const bool different_category = this->get_category() != type->get_category();
	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
	}

	this->type = type;

	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
	}

	if (type->get_hit_points() != old_type->get_hit_points()) {
		this->change_max_hit_points(type->get_hit_points() - old_type->get_hit_points());
	}

	emit type_changed();
}

military_unit_category military_unit::get_category() const
{
	return this->get_type()->get_category();
}

military_unit_domain military_unit::get_domain() const
{
	return this->get_type()->get_domain();
}

const icon *military_unit::get_icon() const
{
	return this->get_type()->get_icon();
}

void military_unit::set_province(const metternich::province *province)
{
	if (province == this->get_province()) {
		return;
	}

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->remove_military_unit(this);
	}

	this->province = province;

	if (this->get_province() != nullptr && this->get_original_province() == nullptr) {
		//if the unit is moving, it will be added to the province when it finishes, otherwise add it now
		this->get_province()->get_game_data()->add_military_unit(this);
	}

	emit province_changed();
}

bool military_unit::can_move_to(const metternich::province *province) const
{
	const country *province_owner = province->get_game_data()->get_owner();
	if (province_owner == this->get_owner()) {
		return true;
	}

	if (province_owner != nullptr) {
		return province_owner->get_game_data()->is_any_vassal_of(this->get_owner());
	}

	return false;
}

void military_unit::move_to(const metternich::province *province)
{
	this->set_original_province(this->get_province());
	this->set_province(province);
}

void military_unit::cancel_move()
{
	assert_throw(this->get_original_province() != nullptr);

	this->set_province(this->get_original_province());
	this->set_original_province(nullptr);
}

void military_unit::set_site(const metternich::site *site)
{
	if (site == this->get_site()) {
		return;
	}

	if (this->get_site() != nullptr) {
		this->get_site()->get_game_data()->remove_visiting_military_unit(this);
	}

	this->site = site;

	if (this->get_site() != nullptr) {
		this->get_site()->get_game_data()->add_visiting_military_unit(this);
	}

	emit site_changed();
}

void military_unit::visit_site(const metternich::site *site)
{
	assert_throw(site != nullptr);
	assert_throw(site->get_game_data()->get_improvement() != nullptr);
	assert_throw(site->get_game_data()->get_improvement()->is_ruins());

	assert_throw(this->get_site() == nullptr);
	assert_throw(this->get_original_province() == nullptr);
	assert_throw(this->get_province() != nullptr);

	this->set_original_province(this->get_province());
	this->set_province(nullptr);
	this->set_site(site);
}

void military_unit::set_hit_points(const int hit_points)
{
	if (hit_points == this->get_hit_points()) {
		return;
	}

	this->hit_points = hit_points;

	assert_throw(this->get_hit_points() <= this->get_max_hit_points());

	if (this->get_morale() > this->get_hit_points()) {
		this->set_morale(this->get_hit_points());
	}

	if (this->get_hit_points() <= 0) {
		this->disband(false);
	}
}

int military_unit::get_morale_resistance() const
{
	int morale_resistance = 0;

	if (this->get_owner() != nullptr) {
		const country_game_data *owner_game_data = this->get_owner()->get_game_data();

		switch (this->get_domain()) {
			case military_unit_domain::land:
				morale_resistance += owner_game_data->get_land_morale_resistance_modifier();
				break;
			case military_unit_domain::water:
				morale_resistance += owner_game_data->get_naval_morale_resistance_modifier();
				break;
			default:
				break;
		}
	}

	//FIXME: add morale resistance from commander

	return morale_resistance;
}

void military_unit::receive_damage(const int damage)
{
	this->change_hit_points(-damage);

	const int morale_damage = damage * (100 - this->get_morale_resistance()) / 100;
	this->change_morale(-morale_damage);
}

void military_unit::heal(const int healing)
{
	const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();

	if (missing_hit_points == 0) {
		return;
	}

	this->change_hit_points(std::min(healing, missing_hit_points));
}

void military_unit::disband(const bool restore_population_unit)
{
	if (this->get_character() != nullptr) {
		this->get_character()->get_game_data()->set_military_unit(nullptr);
	}

	if (this->is_moving()) {
		if (this->get_site() != nullptr) {
			this->get_site()->get_game_data()->remove_visiting_military_unit(this);
		}
	} else {
		if (this->get_province() != nullptr) {
			this->get_province()->get_game_data()->remove_military_unit(this);
		}
	}

	if (this->get_home_province() != nullptr) {
		this->get_home_province()->get_game_data()->remove_home_military_unit(this);

		if (restore_population_unit) {
			assert_throw(this->get_population_type() != nullptr);
			assert_throw(this->get_culture() != nullptr);
			assert_throw(this->get_religion() != nullptr);
			assert_throw(this->get_phenotype() != nullptr);

			this->get_home_province()->get_game_data()->create_population_unit(this->get_population_type(), this->get_culture(), this->get_religion(), this->get_phenotype());
		}
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->remove_military_unit(this);
	}
}

void military_unit::disband()
{
	this->disband(true);
}

}
