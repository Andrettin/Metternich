#pragma once

#include "domain/domain.h"
#include "script/context.h"
#include "script/target_variant.h"

namespace metternich {

using domain_target_variant = target_variant<const domain>;

inline const domain *get_domain_from_target(const domain_target_variant &domain_target, const read_only_context &ctx)
{
	if (std::holds_alternative<const domain *>(domain_target)) {
		return std::get<const domain *>(domain_target);
	} else if (std::holds_alternative<std::string>(domain_target)) {
		return ctx.get_saved_scope<const domain>(std::get<std::string>(domain_target));
	} else if (std::holds_alternative<special_target_type>(domain_target)) {
		const special_target_type target_type = std::get<special_target_type>(domain_target);
		const read_only_context::scope_variant_type &target_scope_variant = ctx.get_special_target_scope_variant(target_type);

		return std::visit([](auto &&target_scope) -> const domain * {
			using target_scope_type = std::remove_const_t<std::remove_pointer_t<std::decay_t<decltype(target_scope)>>>;

			if constexpr (std::is_same_v<target_scope_type, std::monostate>) {
				assert_throw(false);
				return nullptr;
			} else {
				return condition<target_scope_type>::get_scope_domain(target_scope);
			}
		}, target_scope_variant);
	}

	assert_throw(false);
	return nullptr;
}

}
