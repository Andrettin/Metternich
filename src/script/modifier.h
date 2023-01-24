#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;

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

	void process_gsml_scope(const gsml_data &scope)
	{
		Q_UNUSED(scope);
	}

	void apply(scope_type *scope, const int multiplier = 1) const;
	void remove(scope_type *scope, const int multiplier = 1) const;
	std::string get_string(const int multiplier = 1, const size_t indent = 0) const;

private:
	void add_modifier_effect(std::unique_ptr<modifier_effect<scope_type>> &&modifier_effect);

private:
	std::vector<std::unique_ptr<modifier_effect<scope_type>>> modifier_effects;
};

extern template class modifier<const character>;
extern template class modifier<const country>;

}
