#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class site;
struct context;
struct read_only_context;

template <typename scope_type>
class effect;

template <typename scope_type>
class effect_list final
{
public:
	static constexpr const char no_effect_string[] = "No effect";

	effect_list();
	~effect_list();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;
	void do_effects(scope_type *scope, context &ctx) const;
	std::string get_effects_string(const scope_type *scope, const read_only_context &ctx, const size_t indent = 0, const std::string &prefix = "", const bool indent_first_line = true) const;
	void add_effect(std::unique_ptr<effect<scope_type>> &&effect);

private:
	std::vector<std::unique_ptr<effect<scope_type>>> effects;
};

extern template class effect_list<const character>;
extern template class effect_list<const country>;
extern template class effect_list<const site>;

}
