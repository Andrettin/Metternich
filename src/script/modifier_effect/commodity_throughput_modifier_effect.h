#pragma once

#include "domain/country_economy.h"
#include "domain/domain.h"
#include "economy/commodity.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class commodity_throughput_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit commodity_throughput_modifier_effect(const metternich::commodity *commodity, const std::string &value)
		: modifier_effect<scope_type>(value), commodity(commodity)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "commodity_throughput_modifier";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if (!this->commodity->is_enabled()) {
			return;
		}

		scope->get_economy()->change_commodity_throughput_modifier(this->commodity, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Throughput", this->commodity->get_name());
	}

	virtual bool is_percent() const override
	{
		return true;
	}

	virtual bool is_hidden(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return !this->commodity->is_enabled();
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
