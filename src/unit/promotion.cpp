#include "metternich.h"

#include "unit/promotion.h"

#include "database/defines.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "unit/military_unit.h"
#include "unit/military_unit_stat.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

promotion::promotion(const std::string &identifier)
	: named_data_entry(identifier)
{
}

promotion::~promotion()
{
}

void promotion::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<military_unit>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "stat_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const military_unit_stat stat = magic_enum::enum_cast<military_unit_stat>(property.get_key()).value();
			const int value = std::stoi(property.get_value());

			this->stat_bonuses[stat] = value;
		});
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<military_unit>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void promotion::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Promotion \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_modifier() == nullptr && this->get_stat_bonuses().empty()) {
		throw std::runtime_error(std::format("Promotion \"{}\" has no modifier and no stat bonuses.", this->get_identifier()));
	}
}

QCoro::Task<void> promotion::apply_modifier(military_unit *military_unit, const int multiplier) const
{
	for (const auto &[stat, bonus] : this->get_stat_bonuses()) {
		military_unit->change_stat(stat, centesimal_int(bonus * multiplier));
	}

	if (this->get_modifier() != nullptr) {
		co_await this->get_modifier()->apply(military_unit, multiplier);
	}
}

QString promotion::get_modifier_string() const
{
	std::string str;

	for (const auto &[stat, bonus] : this->get_stat_bonuses()) {
		if (!str.empty()) {
			str += "\n";
		}

		const std::string number_str = number::to_signed_string(bonus);
		const QColor &number_color = bonus < 0 ? defines::get()->get_red_text_color() : defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str + (is_percent_military_unit_stat(stat) ? "%" : ""), number_color);

		str += std::format("{}: {}", get_military_unit_stat_name(stat), colored_number_str);
	}

	if (this->get_modifier() != nullptr) {
		if (!str.empty()) {
			str += "\n";
		}

		str += this->get_modifier()->get_string(nullptr);
	}

	return QString::fromStdString(str);
}

}
