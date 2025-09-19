#include "metternich.h"

#include "script/condition/and_condition.h"

namespace metternich {

template class and_condition<character>;
template class and_condition<domain>;
template class and_condition<military_unit>;
template class and_condition<population_unit>;
template class and_condition<province>;
template class and_condition<site>;

}
