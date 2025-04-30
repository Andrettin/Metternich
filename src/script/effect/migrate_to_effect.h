#pragma once

#include "map/site.h"
#include "script/effect/effect.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class migrate_to_effect final : public effect<population_unit>
{
public:
	explicit migrate_to_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<population_unit>(effect_operator)
	{
		this->site_target = string_to_target_variant<const site>(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "migrate_to";
		return class_identifier;
	}

	virtual void do_assignment_effect(population_unit *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const site *site = this->get_site(ctx);
		assert_throw(site->get_game_data()->can_have_population());

		if (!site->get_game_data()->is_built()) {
			return;
		}

		assert_throw(scope->get_settlement() != nullptr);

		scope->migrate_to(site);
	}

	virtual std::string get_assignment_string(const population_unit *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const site *site = this->get_site(ctx);

		return std::format("Migrate to {}", string::highlight(site->get_game_data()->get_current_cultural_name()));
	}

	const site *get_site(const read_only_context &ctx) const
	{
		if (std::holds_alternative<const site *>(this->site_target)) {
			return std::get<const site *>(this->site_target);
		} else if (std::holds_alternative<std::string>(this->site_target)) {
			return ctx.get_saved_scope<const site>(std::get<std::string>(this->site_target));
		} else if (std::holds_alternative<special_target_type>(this->site_target)) {
			const special_target_type target_type = std::get<special_target_type>(this->site_target);
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
	target_variant<const site> site_target;
};

}
