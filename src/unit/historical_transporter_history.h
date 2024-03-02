#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class historical_transporter_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(bool active MEMBER active READ is_active)

public:
	bool is_active() const
	{
		return this->active;
	}

private:
	bool active = false;
};

}
