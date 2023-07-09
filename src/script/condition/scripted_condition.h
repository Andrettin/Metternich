#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class character;
class country;
class population_unit;
class province;
class site;

template <typename scope_type>
class and_condition;

//the class for a predefined, reusable scripted condition
template <typename scope_type>
class scripted_condition_base
{
public:
	explicit scripted_condition_base();
	~scripted_condition_base();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);
	void check() const;

	const and_condition<scope_type> *get_conditions() const
	{
		return this->conditions.get();
	}

private:
	std::unique_ptr<and_condition<scope_type>> conditions;
};

class character_scripted_condition final : public data_entry, public data_type<character_scripted_condition>, public scripted_condition_base<character>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::character_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/character";

	explicit character_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

class country_scripted_condition final : public data_entry, public data_type<country_scripted_condition>, public scripted_condition_base<country>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "country_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::country_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/country";

	explicit country_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

class military_unit_scripted_condition final : public data_entry, public data_type<military_unit_scripted_condition>, public scripted_condition_base<military_unit>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "military_unit_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::military_unit_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/military_unit";

	explicit military_unit_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

class population_unit_scripted_condition final : public data_entry, public data_type<population_unit_scripted_condition>, public scripted_condition_base<population_unit>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "population_unit_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::population_unit_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/population_unit";

	explicit population_unit_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

class province_scripted_condition final : public data_entry, public data_type<province_scripted_condition>, public scripted_condition_base<province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::province_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/province";

	explicit province_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

class site_scripted_condition final : public data_entry, public data_type<site_scripted_condition>, public scripted_condition_base<site>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "site_scripted_condition";
	static constexpr const char property_class_identifier[] = "metternich::site_scripted_condition*";
	static constexpr const char database_folder[] = "scripted_conditions/site";

	explicit site_scripted_condition(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_condition_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_condition_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_condition_base::check();
	}
};

extern template class scripted_condition_base<character>;
extern template class scripted_condition_base<country>;
extern template class scripted_condition_base<population_unit>;
extern template class scripted_condition_base<province>;
extern template class scripted_condition_base<site>;

}
