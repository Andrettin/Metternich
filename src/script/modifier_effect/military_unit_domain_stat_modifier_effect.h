#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "util/string_util.h"

namespace metternich {

class military_unit_domain_stat_modifier_effect final : public modifier_effect<const country>
{
public:
	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_domain_stat_modifier";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "domain") {
			this->domain = enum_converter<military_unit_domain>::to_enum(value);
		} else if (key == "stat") {
			this->stat = enum_converter<military_unit_stat>::to_enum(value);
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	virtual void apply(const country *scope, const centesimal_int &multiplier) const override
	{
		for (const military_unit_type *military_unit_type : military_unit_type::get_all()) {
			if (military_unit_type->get_domain() == this->domain) {
				scope->get_game_data()->change_military_unit_type_stat_modifier(military_unit_type, this->stat, this->value * multiplier);
			}
		}
	}

	virtual std::string get_base_string() const override
	{
		return std::format("{} {}", domain == military_unit_domain::water ? "Naval" : get_military_unit_domain_name(this->domain), get_military_unit_stat_name(this->stat));
	}

	virtual bool are_decimals_relevant() const override
	{
		return true;
	}

	virtual bool is_percent() const override
	{
		return is_percent_military_unit_stat(this->stat);
	}

private:
	military_unit_domain domain{};
	military_unit_stat stat{};
};

}
