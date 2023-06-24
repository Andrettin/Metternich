#include "metternich.h"

#include "map/route.h"
#include "map/route_game_data.h"
#include "map/route_history.h"

namespace metternich {

void route::check() const
{
	if (!this->get_color().isValid()) {
		throw std::runtime_error(std::format("Route \"{}\" has no color.", this->get_identifier()));
	}
}

data_entry_history *route::get_history_base()
{
	return this->history.get();
}

void route::reset_history()
{
	this->history = make_qunique<route_history>(this);
}

void route::reset_game_data()
{
	this->game_data = make_qunique<route_game_data>(this);
}

}
