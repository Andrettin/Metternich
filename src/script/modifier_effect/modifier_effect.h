#pragma once

#include "util/fractional_int.h"

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
class modifier_effect
{
public:
	static std::unique_ptr<modifier_effect> from_gsml_property(const gsml_property &property);
	static std::unique_ptr<modifier_effect> from_gsml_scope(const gsml_data &scope);

	modifier_effect()
	{
	}

	explicit modifier_effect(const std::string &value) : value(centesimal_int(value))
	{
	}

	virtual ~modifier_effect()
	{
	}

	virtual const std::string &get_identifier() const = 0;
	virtual void process_gsml_property(const gsml_property &property);
	virtual void process_gsml_scope(const gsml_data &scope);
	centesimal_int get_multiplied_value(const centesimal_int &multiplier) const;
	virtual void apply(scope_type *scope, const centesimal_int &multiplier) const = 0;

	virtual std::string get_base_string() const = 0;
	virtual std::string get_string(const centesimal_int &multiplier, const bool ignore_decimals) const;

	virtual std::string get_string(const scope_type *scope, const centesimal_int &multiplier, const bool ignore_decimals) const
	{
		Q_UNUSED(scope);

		return this->get_string(multiplier, ignore_decimals);
	}

	virtual bool is_negative(const centesimal_int &multiplier) const
	{
		return (this->value * multiplier) < 0;
	}

	virtual bool is_percent() const
	{
		return false;
	}

	virtual bool is_hidden() const
	{
		return false;
	}

	virtual bool are_decimals_relevant() const
	{
		return false;
	}

protected:
	centesimal_int value;
};

extern template class modifier_effect<const character>;
extern template class modifier_effect<const country>;
extern template class modifier_effect<military_unit>;
extern template class modifier_effect<const province>;
extern template class modifier_effect<const site>;

}
