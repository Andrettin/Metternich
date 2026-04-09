#include "metternich.h"

#include "game/decision.h"

#include "game/decision_type.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"

namespace metternich {

decision::decision(const std::string &identifier) : named_data_entry(identifier)
{
}

decision::~decision()
{
}

void decision::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<domain>>();
		this->conditions->process_gsml_data(scope);
	} else if (tag == "effects") {
		this->effects = std::make_unique<effect_list<const domain>>();
		this->effects->process_gsml_data(scope);
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void decision::check() const
{
	if (this->get_type() == decision_type::none) {
		throw std::runtime_error(std::format("Decision \"{}\" has no type.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Decision \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_effects() == nullptr) {
		throw std::runtime_error(std::format("Decision \"{}\" has no effects.", this->get_identifier()));
	}
}

QString decision::get_conditions_string(const domain *domain) const
{
	Q_UNUSED(domain);

	std::string str;
	if (this->get_conditions() != nullptr) {
		str += "Requirements: " + this->get_conditions()->get_string(0);
	}

	return QString::fromStdString(str);
}

QString decision::get_effects_string(const domain *domain) const
{
	std::string str = this->get_effects()->get_effects_single_line_string(domain, read_only_context(domain));

	return QString::fromStdString(str);
}

bool decision::can_be_enacted_by(const metternich::domain *domain) const
{
	if (this->get_conditions() != nullptr && !this->get_conditions()->check(domain, read_only_context(domain))) {
		return false;
	}

	return true;
}

QCoro::Task<void> decision::enact_for_coro(const metternich::domain *domain) const
{
	context ctx(domain);
	co_await this->get_effects()->do_effects(domain, ctx);
}

}
