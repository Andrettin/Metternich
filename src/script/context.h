#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class country;
class population_unit;
class province;

//script context for e.g. events
template <bool read_only>
struct context_base
{
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	gsml_data to_gsml_data(const std::string &tag) const;

	const country *source_country = nullptr;
	const country *current_country = nullptr;
};

extern template struct context_base<false>;
extern template struct context_base<true>;

struct context final : context_base<false>
{
	static context from_scope(country *country)
	{
		context ctx;
		ctx.current_country = country;
		return ctx;
	}

	static context from_scope(const population_unit *population_unit);
	static context from_scope(const province *province);
};

struct read_only_context final : context_base<true>
{
public:
	static read_only_context from_scope(const country *country)
	{
		read_only_context ctx;
		ctx.current_country = country;
		return ctx;
	}

	static read_only_context from_scope(const population_unit *population_unit);
	static read_only_context from_scope(const province *province);

	read_only_context()
	{
	}

	read_only_context(const context &ctx)
	{
		this->source_country = ctx.source_country;
		this->current_country = ctx.current_country;
	}
};

}
