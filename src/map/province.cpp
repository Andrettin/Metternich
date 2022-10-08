#include "metternich.h"

#include "map/province.h"

#include "map/province_game_data.h"
#include "util/assert_util.h"

namespace metternich {

province::province(const std::string &identifier) : named_data_entry(identifier)
{
}

province::~province()
{
}

void province::check() const
{
	assert_throw(this->get_color().isValid());
}

void province::reset_game_data()
{
	this->game_data = std::make_unique<province_game_data>(this);
}

}
