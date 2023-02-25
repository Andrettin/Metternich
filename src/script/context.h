#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class military_unit;
class population_unit;
class province;
class site;
enum class special_target_type;

//script context for e.g. events
template <bool read_only>
struct context_base
{
	using population_unit_type = std::conditional_t<read_only, const population_unit, population_unit>;
	using population_unit_ptr = population_unit_type *;
	using scope_variant_type = std::variant<std::monostate, const character *, const country *, population_unit_ptr, const province *, const site *>;

	using military_unit_ptr = std::conditional_t<read_only, const military_unit *, military_unit *>;

	context_base()
	{
	}

	explicit context_base(const scope_variant_type &root_scope) : root_scope(root_scope)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	gsml_data to_gsml_data(const std::string &tag) const;

	template <typename scope_type>
	std::map<std::string, scope_type *> &get_saved_scopes()
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			return this->saved_character_scopes;
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			return this->saved_country_scopes;
		} else if constexpr (std::is_same_v<scope_type, population_unit_type>) {
			return this->saved_population_unit_scopes;
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			return this->saved_province_scopes;
		} else if constexpr (std::is_same_v<scope_type, const site>) {
			return this->saved_site_scopes;
		}
	}

	template <typename scope_type>
	const std::map<std::string, scope_type *> &get_saved_scopes() const
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			return this->saved_character_scopes;
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			return this->saved_country_scopes;
		} else if constexpr (std::is_same_v<scope_type, population_unit_type>) {
			return this->saved_population_unit_scopes;
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			return this->saved_province_scopes;
		} else if constexpr (std::is_same_v<scope_type, const site>) {
			return this->saved_site_scopes;
		}
	}

	template <typename scope_type>
	scope_type *get_special_target_scope(const special_target_type special_target_type) const
	{
		switch (special_target_type) {
			case special_target_type::root:
				if (std::holds_alternative<scope_type *>(this->root_scope)) {
					return std::get<scope_type *>(this->root_scope);
				}
				break;
			case special_target_type::source:
				if (std::holds_alternative<scope_type *>(this->source_scope)) {
					return std::get<scope_type *>(this->source_scope);
				}
				break;
			case special_target_type::previous:
				if (std::holds_alternative<scope_type *>(this->previous_scope)) {
					return std::get<scope_type *>(this->previous_scope);
				}
				break;
			default:
				break;
		}

		return nullptr;
	}

	scope_variant_type root_scope = std::monostate();
	scope_variant_type source_scope = std::monostate();
	scope_variant_type previous_scope = std::monostate();
	std::map<std::string, const character *> saved_character_scopes;
	std::map<std::string, const country *> saved_country_scopes;
	std::map<std::string, population_unit_ptr> saved_population_unit_scopes;
	std::map<std::string, const province *> saved_province_scopes;
	std::map<std::string, const site *> saved_site_scopes;
	std::vector<military_unit_ptr> attacking_military_units;
	std::vector<military_unit_ptr> defending_military_units;
};

extern template struct context_base<false>;
extern template struct context_base<true>;

struct context final : context_base<false>
{
	context()
	{
	}

	explicit context(const scope_variant_type &root_scope) : context_base(root_scope)
	{
	}

	template <typename scope_type>
	bool is_root_scope(scope_type *scope) const
	{
		return std::holds_alternative<scope_type *>(this->root_scope) && std::get<scope_type *>(this->root_scope) == scope;
	}
};

struct read_only_context final : context_base<true>
{
public:
	static scope_variant_type scope_from_mutable(const context_base<false>::scope_variant_type &mutable_scope)
	{
		scope_variant_type scope;

		std::visit([&scope](auto &&arg) {
			scope = arg;
		}, mutable_scope);

		return scope;
	}

	read_only_context()
	{
	}

	explicit read_only_context(const scope_variant_type &root_scope) : context_base(root_scope)
	{
	}

	read_only_context(const context &ctx) : read_only_context(read_only_context::scope_from_mutable(ctx.root_scope))
	{
		this->source_scope = read_only_context::scope_from_mutable(ctx.source_scope);
		this->previous_scope = read_only_context::scope_from_mutable(ctx.previous_scope);

		this->saved_character_scopes = ctx.saved_character_scopes;
		this->saved_country_scopes = ctx.saved_country_scopes;
		this->saved_province_scopes = ctx.saved_province_scopes;
		this->saved_site_scopes = ctx.saved_site_scopes;

		for (const auto &[str, population_unit] : ctx.saved_population_unit_scopes) {
			this->saved_population_unit_scopes[str] = population_unit;
		}

		for (const military_unit *military_unit : ctx.attacking_military_units) {
			this->attacking_military_units.push_back(military_unit);
		}

		for (const military_unit *military_unit : ctx.defending_military_units) {
			this->defending_military_units.push_back(military_unit);
		}
	}

	template <typename scope_type>
	bool is_root_scope(const scope_type *scope) const
	{
		return std::holds_alternative<const scope_type *>(this->root_scope) && std::get<const scope_type *>(this->root_scope) == scope;
	}
};

}
