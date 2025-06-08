#pragma once

#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;

class trait_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(bool hidden_name MEMBER hidden_name READ has_hidden_name NOTIFY changed)
	Q_PROPERTY(int max_scaling MEMBER max_scaling READ get_max_scaling NOTIFY changed)

public:
	explicit trait_base(const std::string &identifier);

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_level() const
	{
		return this->level;
	}

	bool has_hidden_name() const
	{
		return this->hidden_name;
	}

	int get_max_scaling() const
	{
		return this->max_scaling;
	}

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	int level = 1;
	bool hidden_name = false;
	int max_scaling = std::numeric_limits<int>::max();
};

}
