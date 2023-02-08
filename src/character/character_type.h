#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "ui/icon_container.h"

namespace metternich {

class country;
class icon;
class province;
enum class attribute;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class character_type final : public named_data_entry, public data_type<character_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::attribute primary_attribute MEMBER primary_attribute NOTIFY changed)
	Q_PROPERTY(int primary_attribute_index READ get_primary_attribute_index CONSTANT)
	Q_PROPERTY(QString primary_attribute_name READ get_primary_attribute_name_qstring CONSTANT)

public:
	static constexpr const char class_identifier[] = "character_type";
	static constexpr const char property_class_identifier[] = "metternich::character_type*";
	static constexpr const char database_folder[] = "character_types";

	explicit character_type(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const icon *get_portrait() const
	{
		return this->portrait;
	}

	const icon_map<std::unique_ptr<const condition<character>>> &get_conditional_portraits() const
	{
		return this->conditional_portraits;
	}

	const condition<character> *get_portrait_conditions(const icon *portrait) const
	{
		const auto find_iterator = this->conditional_portraits.find(portrait);
		if (find_iterator != this->conditional_portraits.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	attribute get_primary_attribute() const
	{
		return this->primary_attribute;
	}

	int get_primary_attribute_index() const
	{
		return static_cast<int>(this->get_primary_attribute());
	}

	QString get_primary_attribute_name_qstring() const;

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

	const modifier<const province> *get_province_modifier() const
	{
		return this->province_modifier.get();
	}

signals:
	void changed();

private:
	icon *portrait = nullptr;
	icon_map<std::unique_ptr<const condition<character>>> conditional_portraits;
	attribute primary_attribute;
	std::unique_ptr<modifier<const country>> country_modifier;
	std::unique_ptr<modifier<const province>> province_modifier;
};

}
