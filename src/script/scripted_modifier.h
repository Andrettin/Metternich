#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class scripted_modifier : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(bool negative MEMBER negative READ is_negative NOTIFY changed)
	Q_PROPERTY(QString modifier_string READ get_modifier_string CONSTANT)

public:
	explicit scripted_modifier(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override
	{
		if (this->get_icon() == nullptr) {
			throw std::runtime_error("Scripted modifier \"" + this->get_identifier() + "\" has no icon.");
		}
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_negative() const
	{
		return this->negative;
	}

	virtual QString get_modifier_string() const = 0;

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	bool negative = false;
};

}
