#include "metternich.h"

#include "economy/commodity.h"

#include "economy/commodity_type.h"
#include "economy/food_type.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "technology/technology.h"
#include "util/assert_util.h"

namespace metternich {

commodity::commodity(const std::string &identifier)
	: named_data_entry(identifier), food_type(food_type::none)
{
}

void commodity::initialize()
{
	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_commodity(this);
	}

	if (this->is_provincial()) {
		this->local = true;
	}

	named_data_entry::initialize();
}

void commodity::check() const
{
	assert_throw(this->get_type() != commodity_type::none);
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_wealth_value() >= 0);

	if (this->is_local()) {
		if (!this->is_abstract()) {
			throw std::runtime_error(std::format("Commodity \"{}\" is local but is not abstract, which is not supported.", this->get_identifier()));
		}

		if (this->is_storable()) {
			throw std::runtime_error(std::format("Commodity \"{}\" is both local and storable, which is not supported.", this->get_identifier()));
		}
	}

	if (this->is_provincial() && !this->is_local()) {
		throw std::runtime_error(std::format("Commodity \"{}\" is provincial but not local.", this->get_identifier()));
	}

	if (!this->is_abstract() && this->is_storable() && this->get_base_price() == 0 && this->get_wealth_value() == 0) {
		throw std::runtime_error(std::format("Non-abstract storable commodity \"{}\" has neither a base price nor a wealth value.", this->get_identifier()));
	}

	if (this->get_base_price() != 0 && this->get_wealth_value() != 0) {
		throw std::runtime_error(std::format("Commodity \"{}\" has both a base price and a wealth value.", this->get_identifier()));
	}
}

bool commodity::is_food() const
{
	return this->get_food_type() != food_type::none;
}

bool commodity::is_enabled() const
{
	if (this->required_game_rule != nullptr && game::get()->get_rules() != nullptr) {
		return game::get()->get_rules()->get_value(this->required_game_rule);
	}

	return true;
}

}
