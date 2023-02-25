#pragma once

namespace metternich {

enum class special_target_type;

template <typename scope_type>
using target_variant = std::variant<std::monostate, scope_type *, special_target_type>;

}
