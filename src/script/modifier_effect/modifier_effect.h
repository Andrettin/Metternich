#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class country;
class military_unit;
class province;

template <typename scope_type>
class modifier_effect
{
public:
	static std::unique_ptr<modifier_effect> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<modifier_effect> from_gsml_scope(const gsml_data &scope);

	virtual ~modifier_effect()
	{
	}

	virtual const std::string &get_identifier() const = 0;
	virtual void process_gsml_property(const gsml_property &property);
	virtual void process_gsml_scope(const gsml_data &scope);
	virtual void apply(scope_type *scope, const int multiplier) const = 0;
	virtual std::string get_string(const int multiplier) const = 0;
	virtual int get_score() const = 0;

	virtual bool is_hidden() const
	{
		return false;
	}
};

extern template class modifier_effect<const character>;
extern template class modifier_effect<const country>;
extern template class modifier_effect<military_unit>;
extern template class modifier_effect<const province>;

}
