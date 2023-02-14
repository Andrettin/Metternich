#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class population_unit;
class province;
class site;

//script context for e.g. events
template <bool read_only>
struct context_base
{
	using scope_variant_type = std::variant<std::monostate, const character *, const country *, std::conditional_t<read_only, const population_unit *, population_unit *>, const province *, const site *>;

	context_base()
	{
	}

	explicit context_base(const scope_variant_type &root_scope) : root_scope(root_scope)
	{
	}

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	gsml_data to_gsml_data(const std::string &tag) const;

	scope_variant_type root_scope = std::monostate();
	scope_variant_type source_scope = std::monostate();
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
	}
};

}
