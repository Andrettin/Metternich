#include "metternich.h"

#include "infrastructure/provincial_building_slot.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/country_building_slot.h"
#include "infrastructure/wonder.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {

provincial_building_slot::provincial_building_slot(const building_slot_type *type, const metternich::province *province)
	: building_slot(type), province(province)
{
	assert_throw(this->get_province() != nullptr);

	connect(this, &building_slot::building_changed, this, &provincial_building_slot::country_modifier_changed);
}


void provincial_building_slot::set_building(const building_type *building)
{
	if (building == this->get_building()) {
		return;
	}

	province_game_data *province_game_data = this->get_province()->get_game_data();

	const building_type *old_building = this->get_building();

	if (old_building != nullptr) {
		province_game_data->on_building_gained(old_building, -1);
	}

	building_slot::set_building(building);

	if (this->get_building() != nullptr) {
		province_game_data->on_building_gained(this->get_building(), 1);
	}
}

bool provincial_building_slot::can_have_building(const building_type *building) const
{
	if (!building->is_provincial()) {
		return false;
	}

	if (building->get_province_conditions() != nullptr) {
		if (!building->get_province_conditions()->check(this->get_province(), read_only_context(this->get_province()))) {
			return false;
		}
	}

	return building_slot::can_have_building(building);
}

void provincial_building_slot::set_wonder(const metternich::wonder *wonder)
{
	if (wonder == this->get_wonder()) {
		return;
	}

	if (wonder != nullptr) {
		assert_throw(wonder->get_building()->get_slot_type() == this->get_type());
	}

	province_game_data *province_game_data = this->get_province()->get_game_data();

	if (this->get_wonder() != nullptr) {
		province_game_data->on_wonder_gained(this->get_wonder(), -1);
	}

	this->wonder = wonder;

	if (this->get_wonder() != nullptr) {
		province_game_data->on_wonder_gained(this->get_wonder(), 1);

		if (this->get_building() == nullptr || this->get_wonder()->get_building()->is_any_required_building(this->get_building())) {
			this->set_building(this->get_wonder()->get_building());
		}
	}

	if (game::get()->is_running()) {
		emit wonder_changed();
	}
}

void provincial_building_slot::set_under_construction_wonder(const metternich::wonder *wonder)
{
	if (wonder == this->get_under_construction_wonder()) {
		return;
	}

	this->under_construction_wonder = wonder;

	if (game::get()->is_running()) {
		emit under_construction_wonder_changed();
	}
}

bool provincial_building_slot::can_have_wonder(const metternich::wonder *wonder) const
{
	if (this->get_wonder() != nullptr) {
		return false;
	}

	if (wonder->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!wonder->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	if (wonder->get_province_conditions() != nullptr) {
		if (!wonder->get_province_conditions()->check(this->get_province(), read_only_context(this->get_province()))) {
			return false;
		}
	}

	if (wonder->get_building()->get_province_conditions() != nullptr) {
		if (!wonder->get_building()->get_province_conditions()->check(this->get_province(), read_only_context(this->get_province()))) {
			return false;
		}
	}

	if (wonder->get_building()->get_conditions() != nullptr) {
		if (this->get_country() == nullptr) {
			return false;
		}

		if (!wonder->get_building()->get_conditions()->check(this->get_country(), read_only_context(this->get_country()))) {
			return false;
		}
	}

	return true;
}

bool provincial_building_slot::can_build_wonder(const metternich::wonder *wonder) const
{
	if (wonder->get_building()->get_required_building() != nullptr && this->get_building() != wonder->get_building()->get_required_building() && !this->get_building()->is_any_required_building(wonder->get_building())) {
		return false;
	}

	return this->can_have_wonder(wonder);
}

wonder *provincial_building_slot::get_buildable_wonder() const
{
	for (const metternich::wonder *wonder : this->get_type()->get_wonders()) {
		if (wonder->get_required_technology() != nullptr) {
			if (this->get_country() == nullptr) {
				continue;
			}

			if (!this->get_country()->get_game_data()->has_technology(wonder->get_required_technology())) {
				continue;
			}
		}

		if (!this->can_build_wonder(wonder)) {
			continue;
		}

		return const_cast<metternich::wonder *>(wonder);
	}

	return nullptr;
}

const country *provincial_building_slot::get_country() const
{
	return this->get_province()->get_game_data()->get_owner();
}

QString provincial_building_slot::get_modifier_string() const
{
	if (this->get_building() == nullptr) {
		return QString();
	}

	assert_throw(this->get_building()->is_provincial());

	std::string str;

	if (this->get_wonder() != nullptr) {
		if (this->get_wonder()->get_country_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += this->get_wonder()->get_country_modifier()->get_string();
		}

		if (this->get_wonder()->get_province_modifier() != nullptr) {
			if (!str.empty()) {
				str += "\n";
			}

			str += this->get_wonder()->get_province_modifier()->get_string();
		}
	}

	if (this->get_building()->get_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		const country_game_data *country_game_data = this->get_country()->get_game_data();
		const centesimal_int multiplier = centesimal_int(1) / country_game_data->get_province_count();
		str += this->get_building()->get_country_modifier()->get_string(multiplier, 0, false);
	}

	if (this->get_building()->get_stackable_country_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_stackable_country_modifier()->get_string();
	}

	if (this->get_building()->get_province_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_building()->get_province_modifier()->get_string();
	}

	return QString::fromStdString(str);
}

}
