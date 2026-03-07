#pragma once

#include "database/named_data_entry.h"

namespace metternich {

class character;

template <typename scope_type>
class modifier;

class character_attribute_base : public named_data_entry
{
	Q_OBJECT

public:
	explicit character_attribute_base(const std::string &identifier);
	~character_attribute_base();

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
	std::map<int, std::unique_ptr<modifier<const character>>> value_modifiers; //the character modifiers applied for each value; these are cumulative
};

}
