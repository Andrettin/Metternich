#include "metternich.h"

#include "map/province.h"

#include "map/province_game_data.h"
#include "map/province_history.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

province::province(const std::string &identifier) : named_data_entry(identifier)
{
	this->reset_game_data();
}

province::~province()
{
}

void province::check() const
{
	if (this->get_capital_settlement() == nullptr) {
		log::log_error("Province \"" + this->get_identifier() + "\" has no capital settlement.");
	}

	assert_throw(this->get_color().isValid());
}

data_entry_history *province::get_history_base()
{
	return this->history.get();
}

void province::reset_history()
{
	this->history = make_qunique<province_history>(this);
}

void province::reset_game_data()
{
	this->game_data = std::make_unique<province_game_data>(this);
}

}
