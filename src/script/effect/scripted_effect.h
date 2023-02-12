#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "script/effect/effect_list.h"

namespace metternich {

class character;
class country;
class population_unit;
class province;

template <typename scope_type>
class effect_list;

//the class for a predefined, reusable scripted effect
template <typename scope_type>
class scripted_effect_base
{
public:
	void process_gsml_property(const gsml_property &property)
	{
		this->effects.process_gsml_property(property);
	}

	void process_gsml_scope(const gsml_data &scope)
	{
		this->effects.process_gsml_scope(scope);
	}

	void check() const
	{
		this->get_effects().check();
	}

	virtual const std::string &get_identifier() const = 0;

	const effect_list<scope_type> &get_effects() const
	{
		return this->effects;
	}

private:
	effect_list<scope_type> effects;
};

class character_scripted_effect final : public data_entry, public data_type<character_scripted_effect>, public scripted_effect_base<const character>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_scripted_effect";
	static constexpr const char property_class_identifier[] = "metternich::character_scripted_effect*";
	static constexpr const char database_folder[] = "scripted_effects/character";

	explicit character_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class country_scripted_effect final : public data_entry, public data_type<country_scripted_effect>, public scripted_effect_base<const country>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "country_scripted_effect";
	static constexpr const char property_class_identifier[] = "metternich::country_scripted_effect*";
	static constexpr const char database_folder[] = "scripted_effects/country";

	explicit country_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class population_unit_scripted_effect final : public data_entry, public data_type<population_unit_scripted_effect>, public scripted_effect_base<population_unit>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "population_unit_scripted_effect";
	static constexpr const char property_class_identifier[] = "metternich::population_unit_scripted_effect*";
	static constexpr const char database_folder[] = "scripted_effects/population_unit";

	explicit population_unit_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class province_scripted_effect final : public data_entry, public data_type<province_scripted_effect>, public scripted_effect_base<const province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province_scripted_effect";
	static constexpr const char property_class_identifier[] = "metternich::province_scripted_effect*";
	static constexpr const char database_folder[] = "scripted_effects/province";

	explicit province_scripted_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_effect_base::process_gsml_scope(scope);
	}

	virtual void check() const override
	{
		scripted_effect_base::check();
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

}
