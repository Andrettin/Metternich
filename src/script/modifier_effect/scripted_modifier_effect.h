#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"
#include "script/modifier.h"

namespace metternich {

class character;
class domain;
class military_unit;
class province;
class site;

template <typename scope_type>
class modifier;

//the class for a predefined, reusable scripted modifier effect
template <typename scope_type>
class scripted_modifier_effect_base
{
public:
	void process_gsml_property(const gsml_property &property)
	{
		this->modifier.process_gsml_property(property);
	}

	void process_gsml_scope(const gsml_data &scope)
	{
		this->modifier.process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const = 0;

	const modifier<scope_type> &get_modifier() const
	{
		return this->modifier;
	}

private:
	metternich::modifier<scope_type> modifier;
};

class character_scripted_modifier_effect final : public data_entry, public data_type<character_scripted_modifier_effect>, public scripted_modifier_effect_base<const character>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_scripted_modifier_effect";
	static constexpr const char property_class_identifier[] = "metternich::character_scripted_modifier_effect*";
	static constexpr const char database_folder[] = "scripted_modifier_effects/character";

	explicit character_scripted_modifier_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_modifier_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_modifier_effect_base::process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class domain_scripted_modifier_effect final : public data_entry, public data_type<domain_scripted_modifier_effect>, public scripted_modifier_effect_base<const domain>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "domain_scripted_modifier_effect";
	static constexpr const char property_class_identifier[] = "metternich::domain_scripted_modifier_effect*";
	static constexpr const char database_folder[] = "scripted_modifier_effects/domain";

	explicit domain_scripted_modifier_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_modifier_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_modifier_effect_base::process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class military_unit_scripted_modifier_effect final : public data_entry, public data_type<military_unit_scripted_modifier_effect>, public scripted_modifier_effect_base<military_unit>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "military_unit_scripted_modifier_effect";
	static constexpr const char property_class_identifier[] = "metternich::military_unit_scripted_modifier_effect*";
	static constexpr const char database_folder[] = "scripted_modifier_effects/military_unit";

	explicit military_unit_scripted_modifier_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_modifier_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_modifier_effect_base::process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class province_scripted_modifier_effect final : public data_entry, public data_type<province_scripted_modifier_effect>, public scripted_modifier_effect_base<const province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "province_scripted_modifier_effect";
	static constexpr const char property_class_identifier[] = "metternich::province_scripted_modifier_effect*";
	static constexpr const char database_folder[] = "scripted_modifier_effects/province";

	explicit province_scripted_modifier_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_modifier_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_modifier_effect_base::process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

class site_scripted_modifier_effect final : public data_entry, public data_type<site_scripted_modifier_effect>, public scripted_modifier_effect_base<const site>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "site_scripted_modifier_effect";
	static constexpr const char property_class_identifier[] = "metternich::site_scripted_modifier_effect*";
	static constexpr const char database_folder[] = "scripted_modifier_effects/site";

	explicit site_scripted_modifier_effect(const std::string &identifier) : data_entry(identifier)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		scripted_modifier_effect_base::process_gsml_property(property);
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		scripted_modifier_effect_base::process_gsml_scope(scope);
	}

	virtual const std::string &get_identifier() const override
	{
		return data_entry::get_identifier();
	}
};

}
