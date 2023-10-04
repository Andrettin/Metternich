#include "metternich.h"

#include "country/policy.h"

#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {

policy::policy(const std::string &identifier) : data_entry(identifier)
{
}

policy::~policy()
{
}

void policy::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else if (tag == "left_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->left_modifier = std::move(modifier);
	} else if (tag == "right_modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->right_modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void policy::check() const
{
	if (this->get_modifier() == nullptr) {
		throw std::runtime_error(std::format("Policy \"{}\" has no modifier.", this->get_identifier()));
	}
}

void policy::apply_modifier(const country *country, const int value, const int multiplier) const
{
	if (this->get_modifier() != nullptr) {
		this->get_modifier()->apply(country, value * multiplier);
	}

	if (this->get_left_modifier() != nullptr && value < 0) {
		this->get_left_modifier()->apply(country, std::abs(value) * multiplier);
	}

	if (this->get_right_modifier() != nullptr && value > 0) {
		this->get_right_modifier()->apply(country, value * multiplier);
	}
}

QString policy::get_modifier_string(const country *country, const int value) const
{
	std::string str;

	if (this->get_modifier() != nullptr) {
		str += this->get_modifier()->get_string(country, value);
	}

	if (this->get_left_modifier() != nullptr && value < 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_left_modifier()->get_string(country, std::abs(value));
	}

	if (this->get_right_modifier() != nullptr && value > 0) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_right_modifier()->get_string(country, value);
	}

	return QString::fromStdString(str);
}

}
