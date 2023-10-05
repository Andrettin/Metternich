#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;

	template <int N>
	class fractional_int;

	using centesimal_int = fractional_int<2>;
}

namespace metternich {

class character;
class country;
class military_unit;
class province;
class site;

template <typename scope_type>
class modifier_effect;

//a modifier (i.e. a collection of modifier effects)
template <typename scope_type>
class modifier final
{
public:
	modifier();
	virtual ~modifier();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	void apply(scope_type *scope, const centesimal_int &multiplier) const;
	void apply(scope_type *scope, const int multiplier = 1) const;
	void remove(scope_type *scope, const int multiplier = 1) const;
	std::string get_string(const scope_type *scope, const centesimal_int &multiplier, const size_t indent = 0, const bool ignore_decimals = true) const;
	std::string get_string(const scope_type *scope, const int multiplier = 1, const size_t indent = 0) const;

	void add_modifier_effect(std::unique_ptr<modifier_effect<scope_type>> &&modifier_effect);

private:
	std::vector<std::unique_ptr<modifier_effect<scope_type>>> modifier_effects;
};

extern template class modifier<const character>;
extern template class modifier<const country>;
extern template class modifier<military_unit>;
extern template class modifier<const province>;
extern template class modifier<const site>;

}
