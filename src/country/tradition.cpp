#include "metternich.h"

#include "country/tradition.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/tradition_category.h"
#include "country/tradition_group.h"
#include "game/game.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "technology/technology.h"

namespace metternich {

tradition::tradition(const std::string &identifier) : named_data_entry(identifier)
{
}

tradition::~tradition()
{
}

void tradition::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(tradition::get(value));
		}
	} else if (tag == "incompatible_traditions") {
		for (const std::string &value : values) {
			this->incompatible_traditions.push_back(tradition::get(value));
		}
	} else if (tag == "preconditions") {
		auto preconditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(preconditions, scope);
		this->preconditions = std::move(preconditions);
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<country>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void tradition::initialize()
{
	this->calculate_total_prerequisite_depth();

	if (this->group != nullptr) {
		this->group->add_tradition(this);
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_tradition(this);
	}

	named_data_entry::initialize();
}

void tradition::check() const
{
	if (this->get_category() == tradition_category::none) {
		throw std::runtime_error(std::format("Tradition \"{}\" has no tradition category.", this->get_identifier()));
	}

	if (this->get_group() == nullptr) {
		throw std::runtime_error(std::format("Tradition \"{}\" has no tradition group.", this->get_identifier()));
	}

	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Tradition \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Tradition \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Tradition \"{}\" has no modifier.", this->get_identifier()));
	}

	if (this->get_preconditions() != nullptr) {
		this->get_preconditions()->check_validity();
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}
}

QString tradition::get_category_name_qstring() const
{
	return QString::fromUtf8(get_tradition_category_name(this->get_category()).data());
}

void tradition::calculate_total_prerequisite_depth()
{
	if (this->total_prerequisite_depth != 0 || this->get_prerequisites().empty()) {
		return;
	}

	int depth = 0;

	for (tradition *prerequisite : this->get_prerequisites()) {
		prerequisite->calculate_total_prerequisite_depth();
		depth = std::max(depth, prerequisite->get_total_prerequisite_depth() + 1);
	}

	this->total_prerequisite_depth = depth;
}

QString tradition::get_requirements_string(const metternich::country *country) const
{
	std::string str;

	if (!this->get_incompatible_traditions().empty()) {
		str += "Incompatible with: ";

		bool first = true;
		for (const tradition *tradition : this->get_incompatible_traditions()) {
			if (!tradition->is_available_for_country(country)) {
				continue;
			}

			if (first) {
				first = false;
			} else {
				str += ", ";
			}

			str += tradition->get_name();
		}
	}

	if (!country->get_game_data()->has_tradition(this)) {
		if (this->get_required_technology() != nullptr && !country->get_game_data()->has_technology(this->get_required_technology())) {
			if (!str.empty()) {
				str += "\n\n";
			}

			str += std::format("Required Technology: {}", this->get_required_technology()->get_name());
		}

		if (this->get_conditions() != nullptr && !this->get_conditions()->check(country, read_only_context(country))) {
			if (!str.empty()) {
				str += "\n\n";
			}

			str += this->get_conditions()->get_string(0);
		}
	}

	return QString::fromStdString(str);
}

QString tradition::get_modifier_string(const metternich::country *country) const
{
	return QString::fromStdString(this->get_modifier()->get_string(country));
}

bool tradition::is_available_for_country(const country *country) const
{
	if (this->get_preconditions() != nullptr && !this->get_preconditions()->check(country, read_only_context(country))) {
		return false;
	}

	return true;
}

bool tradition::is_hidden_in_tree() const
{
	return !this->is_available_for_country(game::get()->get_player_country());
}

}
