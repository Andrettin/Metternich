#pragma once

namespace archimedes {
	class gsml_property;
}

namespace metternich {

class character;
class country;
class province;

template <typename scope_type>
class modifier_effect
{
public:
	static std::unique_ptr<modifier_effect> from_gsml_property(const gsml_property &property);

	virtual ~modifier_effect() {}

	virtual const std::string &get_identifier() const = 0;
	virtual void apply(scope_type *scope, const int multiplier) const = 0;
	virtual std::string get_string(const int multiplier) const = 0;
};

extern template class modifier_effect<const character>;
extern template class modifier_effect<const country>;
extern template class modifier_effect<const province>;

}
