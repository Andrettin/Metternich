#pragma once

#include "database/named_data_entry.h"

namespace metternich {

class domain;
enum class idea_type;

template <typename scope_type>
class and_condition;

class idea_slot : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::idea_type idea_type READ get_idea_type CONSTANT)

public:
	explicit idea_slot(const std::string &identifier);
	~idea_slot();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	virtual idea_type get_idea_type() const = 0;

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	std::unique_ptr<const and_condition<domain>> conditions;
};

}
