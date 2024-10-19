#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "script/condition/scripted_condition_base.h"

namespace metternich {

class character;
class country;
class population_unit;
class province;
class site;
struct read_only_context;

class character_scripted_condition final : public data_entry, public data_type<character_scripted_condition>, public scripted_condition_base<character, read_only_context>
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

class country_scripted_condition final : public data_entry, public data_type<country_scripted_condition>, public scripted_condition_base<country, read_only_context>
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

class military_unit_scripted_condition final : public data_entry, public data_type<military_unit_scripted_condition>, public scripted_condition_base<military_unit, read_only_context>
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

class population_unit_scripted_condition final : public data_entry, public data_type<population_unit_scripted_condition>, public scripted_condition_base<population_unit, read_only_context>
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

class province_scripted_condition final : public data_entry, public data_type<province_scripted_condition>, public scripted_condition_base<province, read_only_context>
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

class site_scripted_condition final : public data_entry, public data_type<site_scripted_condition>, public scripted_condition_base<site, read_only_context>
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

}
