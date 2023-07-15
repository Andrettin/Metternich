#pragma once

#include "database/data_type.h"
#include "script/scripted_modifier.h"
#include "script/scripted_scoped_modifier.h"

namespace metternich {

class province;

template <typename scope_type>
class modifier;

class scripted_province_modifier final : public scripted_modifier, public data_type<scripted_province_modifier>, public scripted_scoped_modifier<province>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "scripted_province_modifier";
	static constexpr const char property_class_identifier[] = "metternich::scripted_province_modifier*";
	static constexpr const char database_folder[] = "scripted_modifiers/province";

	explicit scripted_province_modifier(const std::string &identifier) : scripted_modifier(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		if (!scripted_scoped_modifier::process_gsml_scope(scope)) {
			data_entry::process_gsml_scope(scope);
		}
	}

	virtual void check() const override
	{
		scripted_modifier::check();
		scripted_scoped_modifier::check();
	}

	Q_INVOKABLE QString get_modifier_string(metternich::province *scope) const
	{
		return scripted_scoped_modifier::get_modifier_string(scope);
	}
};

}
