#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;

class scripted_modifier : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(bool negative MEMBER negative READ is_negative NOTIFY changed)

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

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	bool negative = false;
};

}
