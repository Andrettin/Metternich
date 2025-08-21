#include "metternich.h"

#include "database/data_entry_container.h"

#include "country/cultural_group.h"
#include "country/idea_slot.h"
#include "country/office.h"
#include "economy/commodity_unit.h"
#include "game/game_rule.h"
#include "technology/technology_category.h"
#include "technology/technology_subcategory.h"
#include "unit/civilian_unit_type.h"

namespace metternich {

template <typename T>
bool data_entry_compare<T>::operator()(const T *lhs, const T *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

template struct data_entry_compare<civilian_unit_type>;
template struct data_entry_compare<commodity_unit>;
template struct data_entry_compare<cultural_group>;
template struct data_entry_compare<game_rule>;
template struct data_entry_compare<idea_slot>;
template struct data_entry_compare<named_data_entry>;
template struct data_entry_compare<office>;
template struct data_entry_compare<technology_category>;
template struct data_entry_compare<technology_subcategory>;

}
