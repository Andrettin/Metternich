#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "domain/country.h"
#include "domain/country_military.h"
#include "script/modifier_effect/modifier_effect.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/string_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

template <typename scope_type>  
class military_unit_stat_modifier_effect final : public modifier_effect<scope_type>
{
public:
	military_unit_stat_modifier_effect() = default;

	explicit military_unit_stat_modifier_effect(const military_unit_stat stat, const std::string &value)
		: modifier_effect<scope_type>(value), stat(stat)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "military_unit_stat";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if constexpr (std::is_same_v<scope_type, const character> || std::is_same_v<scope_type, const country>) {
			if (key == "domain") {
				this->domain = magic_enum::enum_cast<military_unit_domain>(value).value();
				return;
			} else if (key == "category") {
				this->category = magic_enum::enum_cast<military_unit_category>(value).value();
				return;
			} else if (key == "type") {
				this->type = military_unit_type::get(value);
				return;
			}
		}

		if (key == "stat") {
			this->stat = magic_enum::enum_cast<military_unit_stat>(value).value();
		} else if (key == "value") {
			this->value = centesimal_int(value);
		} else {
			modifier_effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void apply(scope_type *scope, const centesimal_int &multiplier) const override
	{
		if (this->domain != military_unit_domain::none) {
			std::vector<const military_unit_type *> types = military_unit_type::get_all_const();
			std::erase_if(types, [this](const military_unit_type *type) {
				return type->get_domain() != this->domain;
			});

			this->apply_to_types(scope, types, multiplier);
		} else if (this->category != military_unit_category::none) {
			std::vector<const military_unit_type *> types = military_unit_type::get_all_const();
			std::erase_if(types, [this](const military_unit_type *type) {
				return type->get_category() != this->category;
			});

			this->apply_to_types(scope, types, multiplier);
		} else if (this->type != nullptr) {
			this->apply_to_types(scope, { this->type }, multiplier);
		} else {
			if constexpr (std::is_same_v<scope_type, const character>) {
				scope->get_game_data()->change_commanded_military_unit_stat_modifier(this->stat, this->value * multiplier);
			} else if constexpr (std::is_same_v<scope_type, military_unit>) {
				scope->change_stat(this->stat, this->value * multiplier);
			} else {
				assert_throw(false);
			}
		}
	}

	void apply_to_types(scope_type *scope, const std::vector<const military_unit_type *> &types, const centesimal_int &multiplier) const
	{
		for (const military_unit_type *military_unit_type : types) {
			if constexpr (std::is_same_v<scope_type, const character>) {
				scope->get_game_data()->change_commanded_military_unit_type_stat_modifier(military_unit_type, this->stat, this->value * multiplier);
			} else if constexpr (std::is_same_v<scope_type, const country>) {
				scope->get_military()->change_military_unit_type_stat_modifier(military_unit_type, this->stat, this->value * multiplier);
			} else {
				assert_throw(false);
			}
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		if (this->domain != military_unit_domain::none) {
			return std::format("{} {}", this->domain == military_unit_domain::water ? "Naval" : get_military_unit_domain_name(this->domain), get_military_unit_stat_name(this->stat));
		} else if (this->category != military_unit_category::none) {
			return std::format("{} {}", get_military_unit_category_name(this->category), get_military_unit_stat_name(this->stat));
		} else if (this->type != nullptr) {
			return std::format("{} {}", this->type->get_name(), get_military_unit_stat_name(this->stat));
		} else {
			return std::string(get_military_unit_stat_name(this->stat));
		}
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
	military_unit_category category{};
	const military_unit_type *type = nullptr;
	military_unit_stat stat{};
};

}
