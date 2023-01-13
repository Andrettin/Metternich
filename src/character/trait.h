#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character_type;
class icon;
enum class attribute;
enum class trait_type;

class trait final : public named_data_entry, public data_type<trait>
{
	Q_OBJECT

	Q_PROPERTY(metternich::trait_type type MEMBER type NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "trait";
	static constexpr const char property_class_identifier[] = "metternich::trait*";
	static constexpr const char database_folder[] = "traits";

	explicit trait(const std::string &identifier);

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

	const std::map<attribute, int> &get_attribute_bonuses() const
	{
		return this->attribute_bonuses;
	}

	bool is_available_for_character_type(const character_type *character_type) const
	{
		if (this->character_types.empty()) {
			return true;
		}

		return this->character_types.contains(character_type);
	}

signals:
	void changed();

private:
	trait_type type;
	metternich::icon *icon = nullptr;
	std::set<const character_type *> character_types; //character types for which this trait is available
	std::map<attribute, int> attribute_bonuses;
};

}
