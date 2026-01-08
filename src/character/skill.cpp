#include "metternich.h"

#include "character/skill.h"

#include "character/character_attribute.h"
#include "character/skill_group.h"
#include "util/assert_util.h"

namespace metternich {

void skill::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "groups") {
		for (const std::string &value : values) {
			skill_group *group = skill_group::get(value);
			group->add_skill(this);
		}
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void skill::initialize()
{
	if (this->base_attribute != nullptr) {
		this->base_attribute->add_derived_skill(this);
	}

	if (this->find_traps) {
		assert_throw(skill::find_traps_skill == nullptr);
		skill::find_traps_skill = this;
	}

	if (this->disarm_traps) {
		assert_throw(skill::disarm_traps_skill == nullptr);
		skill::disarm_traps_skill = this;
	}

	named_data_entry::initialize();
}

void skill::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Skill \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_check_dice().is_null()) {
		throw std::runtime_error(std::format("Skill \"{}\" has no check dice.", this->get_identifier()));
	}

	if (this->get_check_dice().get_count() != 1) {
		throw std::runtime_error(std::format("Skill \"{}\" has check dice with a dice count different than 1.", this->get_identifier()));
	}
}
const metternich::icon *skill::get_icon() const
{
	return this->icon;
}

const dice &skill::get_check_dice() const
{
	return this->check_dice;
}

std::string_view skill::get_value_suffix() const
{
	if (this->get_check_dice().get_sides() == 100) {
		return "%";
	}

	return "";
}

}
