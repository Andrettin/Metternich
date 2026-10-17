#include "metternich.h"

#include "map/province_turn_data.h"

namespace metternich {

province_turn_data::province_turn_data(const metternich::province *province) : province(province)
{
}

province_turn_data::~province_turn_data()
{
}

}

#include "moc_province_turn_data.cpp"
