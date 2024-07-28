#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character;
class country;
class icon;
class military_unit;
enum class trait_type;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class trait final : public named_data_entry, public data_type<trait>
{
	Q_OBJECT

	Q_PROPERTY(metternich::trait_type type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)
	Q_PROPERTY(QString military_unit_modifier_string READ get_military_unit_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "trait";
	static constexpr const char property_class_identifier[] = "metternich::trait*";
	static constexpr const char database_folder[] = "traits";

	explicit trait(const std::string &identifier);
	~trait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	trait_type get_type() const
	{
		return this->type;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const condition<character> *get_conditions() const
	{
		return this->conditions.get();
	}

	const condition<character> *get_generation_conditions() const
	{
		return this->generation_conditions.get();
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	QString get_modifier_string() const;

	const metternich::modifier<const country> *get_ruler_modifier() const
	{
		return this->ruler_modifier.get();
	}

	Q_INVOKABLE QString get_ruler_modifier_string(const metternich::country *country) const;

	const metternich::modifier<military_unit> *get_military_unit_modifier() const
	{
		return this->military_unit_modifier.get();
	}

	QString get_military_unit_modifier_string() const;

signals:
	void changed();

private:
	trait_type type;
	metternich::icon *icon = nullptr;
	std::unique_ptr<const condition<character>> conditions;
	std::unique_ptr<const condition<character>> generation_conditions;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	std::unique_ptr<const metternich::modifier<const country>> ruler_modifier;
	std::unique_ptr<const metternich::modifier<military_unit>> military_unit_modifier;
};

}
