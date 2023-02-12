#include "metternich.h"

#include "unit/military_unit.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/religion.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "unit/military_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

military_unit::military_unit(const military_unit_type *type, const country *owner, const metternich::province *home_province, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype)
	: type(type), owner(owner), home_province(home_province), population_type(population_type), culture(culture), religion(religion), phenotype(phenotype)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_home_province() != nullptr);
	assert_throw(this->get_population_type() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	this->get_home_province()->get_game_data()->add_home_military_unit(this);

	connect(this, &military_unit::type_changed, this, &military_unit::icon_changed);
}

void military_unit::do_turn()
{
	if (this->is_moving()) {
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

	const bool different_category = this->get_category() != type->get_category();
	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
	}

	this->type = type;

	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
	}

	emit type_changed();
}

military_unit_category military_unit::get_category() const
{
	return this->get_type()->get_category();
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

	if (this->get_province() != nullptr && this->original_province == nullptr) {
		//if the unit is moving, it will be added to the province when it finishes, otherwise add it now
		this->get_province()->get_game_data()->add_military_unit(this);
	}

	emit province_changed();
}

bool military_unit::can_move_to(metternich::province *province) const
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

void military_unit::move_to(metternich::province *province)
{
	this->set_original_province(this->get_province());
	this->set_province(province);
}

void military_unit::cancel_move()
{
	assert_throw(this->original_province != nullptr);

	this->set_province(this->original_province);
	this->set_original_province(nullptr);
}

void military_unit::disband(const bool restore_population_unit)
{
	assert_throw(this->get_province() != nullptr);

	this->get_province()->get_game_data()->remove_military_unit(this);

	assert_throw(this->get_home_province() != nullptr);
	this->get_home_province()->get_game_data()->remove_home_military_unit(this);

	if (restore_population_unit) {
		this->get_home_province()->get_game_data()->create_population_unit(this->get_population_type(), this->get_culture(), this->get_religion(), this->get_phenotype());
	}

	this->get_owner()->get_game_data()->remove_military_unit(this);
}

void military_unit::disband()
{
	this->disband(true);
}

}
