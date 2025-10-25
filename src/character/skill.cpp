#include "metternich.h"

#include "character/skill.h"

#include "util/assert_util.h"

namespace metternich {

void skill::initialize()
{
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
