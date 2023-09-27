#pragma once

namespace archimedes {
	class gsml_data;
}

namespace metternich {

class character;
class country;
class province;
class site;

template <typename scope_type>
class modifier;

template <typename scope_type>
class scripted_scoped_modifier
{
public:
	explicit scripted_scoped_modifier();
	~scripted_scoped_modifier();

	bool process_gsml_scope(const gsml_data &scope);
	void check() const;

	const modifier<const scope_type> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string(const scope_type *scope) const;

private:
	std::unique_ptr<modifier<const scope_type>> modifier;
};

extern template class scripted_scoped_modifier<character>;
extern template class scripted_scoped_modifier<country>;
extern template class scripted_scoped_modifier<province>;
extern template class scripted_scoped_modifier<site>;

}
