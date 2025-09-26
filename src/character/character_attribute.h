#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;

template <typename scope_type>
class modifier;

class character_attribute final : public named_data_entry, public data_type<character_attribute>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_attribute";
	static constexpr const char property_class_identifier[] = "metternich::character_attribute*";
	static constexpr const char database_folder[] = "character_attributes";

	explicit character_attribute(const std::string &identifier);
	~character_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const modifier<const character> *get_value_modifier(const int value) const
	{
		const auto find_iterator = this->value_modifiers.find(value);
		if (find_iterator != this->value_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

signals:
	void changed();

private:
	std::map<int, std::unique_ptr<const modifier<const character>>> value_modifiers; //the character modifiers applied for each value; these are cumulative
};

}
