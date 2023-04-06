#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "ui/icon_container.h"

namespace metternich {

class character;
class country;
class icon;
enum class military_unit_category;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class character_type final : public named_data_entry, public data_type<character_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::military_unit_category military_unit_category MEMBER military_unit_category READ get_military_unit_category)
	Q_PROPERTY(bool commander MEMBER commander READ is_commander)

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

	metternich::military_unit_category get_military_unit_category() const
	{
		return this->military_unit_category;
	}

	bool is_commander() const
	{
		return this->commander;
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

signals:
	void changed();

private:
	icon *portrait = nullptr;
	icon_map<std::unique_ptr<const condition<character>>> conditional_portraits;
	metternich::military_unit_category military_unit_category;
	bool commander = false;
	std::unique_ptr<modifier<const country>> country_modifier;
};

}
