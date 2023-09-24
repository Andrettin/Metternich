#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "infrastructure/building_class.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class has_building_class_condition final : public condition<scope_type>
{
public:
	explicit has_building_class_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->building_class = building_class::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_building_class";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, country>) {
			for (const province *province : scope->get_game_data()->get_provinces()) {
				for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
					if (!settlement->get_game_data()->is_built()) {
						continue;
					}

					if (settlement->get_game_data()->has_building_class(this->building_class)) {
						return true;
					}
				}
			}

			return false;
		} else if constexpr (std::is_same_v<scope_type, province>) {
			for (const site *settlement : scope->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				if (settlement->get_game_data()->has_building_class(this->building_class)) {
					return true;
				}
			}

			return false;
		} else if constexpr (std::is_same_v<scope_type, site>) {
			return scope->get_game_data()->has_building_class(this->building_class);
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->building_class->get_name() + " building class";
	}

private:
	const metternich::building_class *building_class = nullptr;
};

}
