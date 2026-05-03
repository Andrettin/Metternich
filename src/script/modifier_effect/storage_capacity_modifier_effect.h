#pragma once

#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "economy/commodity.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class storage_capacity_modifier_effect final : public modifier_effect<const domain>
{
public:
	storage_capacity_modifier_effect() = default;

	explicit storage_capacity_modifier_effect(const std::string &value) : modifier_effect(value)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "storage_capacity";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "commodity") {
			this->commodity = commodity::get(value);
		} else if (key == "capacity") {
			this->value = centesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const domain *scope, const centesimal_int &multiplier) const override
	{
		if (this->commodity != nullptr) {
			co_await scope->get_economy()->change_commodity_storage_capacity(this->commodity, (this->value * multiplier).to_int64());
		} else {
			scope->get_economy()->change_storage_capacity((this->value * multiplier).to_int64());
		}
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		if (this->commodity != nullptr) {
			if (this->commodity->is_manpower()) {
				return std::format("Maximum {}", this->commodity->get_name());
			} else {
				return std::format("{} Storage Capacity", this->commodity->get_name());
			}
		} else {
			return "Storage Capacity";
		}
	}

private:
	const metternich::commodity *commodity = nullptr;
};

}
