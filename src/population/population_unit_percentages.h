#pragma once

#include "util/decimillesimal_int.h"

namespace metternich {

struct population_unit_percentages final
{
	decimillesimal_int literacy_rate;
	decimillesimal_int sick_percent;
	decimillesimal_int starving_percent;
	int fulfilled_everyday_needs_percent = 100;
	int fulfilled_luxury_needs_percent = 100;
};

}
