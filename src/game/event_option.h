#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class province;
class trait;
struct context;
struct read_only_context;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

template <typename scope_type> 
class event_option final
{
public:
	explicit event_option();
	~event_option();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	std::string get_tooltip(const read_only_context &ctx) const;

	int get_ai_weight() const
	{
		return this->ai_weight;
	}

	const condition<std::remove_const_t<scope_type>> *get_conditions() const
	{
		return this->conditions.get();
	}

	std::string get_effects_string(const read_only_context &ctx) const;
	void do_effects(scope_type *scope, context &ctx) const;

private:
	std::string name;
	std::string tooltip;
	const trait *tooltip_info_trait = nullptr;
	int ai_weight = 1;
	std::unique_ptr<condition<std::remove_const_t<scope_type>>> conditions;
	std::unique_ptr<effect_list<scope_type>> effects;
};

extern template class event_option<const character>;
extern template class event_option<const country>;
extern template class event_option<const province>;

}
