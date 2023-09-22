#pragma once

#include "map/site.h"
#include "script/effect/effect.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/enum_converter.h"
#include "util/string_util.h"

namespace metternich {

class migrate_to_effect final : public effect<population_unit>
{
public:
	explicit migrate_to_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<population_unit>(effect_operator)
	{
		if (enum_converter<special_target_type>::has_value(value)) {
			this->settlement_target = enum_converter<special_target_type>::to_enum(value);
		} else {
			this->settlement_target = site::get(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "migrate_to";
		return class_identifier;
	}

	virtual void do_assignment_effect(population_unit *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const site *settlement = this->get_settlement(ctx);
		scope->migrate_to(settlement);
	}

	virtual std::string get_assignment_string(const population_unit *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const site *settlement = this->get_settlement(ctx);

		return std::format("Migrate to {}", string::highlight(settlement->get_name()));
	}

	const site *get_settlement(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const site *>(this->settlement_target)) {
			return std::get<const site *>(this->settlement_target);
		} else if (std::holds_alternative<special_target_type>(this->settlement_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->settlement_target);
			const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

			return std::visit([](auto &&target_scope) -> const site * {
				using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

				if constexpr (std::is_same_v<target_scope_type, const site>) {
					return target_scope;
				} else {
					assert_throw(false);
					return nullptr;
				}
			}, target_scope_variant);
		}

		assert_throw(false);
		return nullptr;
	}

private:
	target_variant<const site> settlement_target;
};

}
