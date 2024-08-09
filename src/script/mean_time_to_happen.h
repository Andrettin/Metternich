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
class province;

template <typename scope_type>
class factor;

template <typename scope_type>
class mean_time_to_happen final
{
public:
	mean_time_to_happen();
	~mean_time_to_happen();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;

	centesimal_int calculate(const scope_type *scope, const int current_year) const;

private:
	std::unique_ptr<factor<scope_type>> factor;
	centesimal_int months;
};

extern template class mean_time_to_happen<character>;
extern template class mean_time_to_happen<country>;
extern template class mean_time_to_happen<province>;

}
