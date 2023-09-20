#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("infrastructure/pathway.h")

namespace metternich {

class pathway;
class route;

class route_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::pathway* pathway MEMBER pathway)

public:
	explicit route_history(const metternich::route *route) : route(route)
	{
	}

	const metternich::pathway *get_pathway() const
	{
		return this->pathway;
	}

private:
	const metternich::route *route = nullptr;
	metternich::pathway *pathway = nullptr;
};

}
