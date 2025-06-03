#include "metternich.h"

#include "database/data_entry_container.h"

#include "country/cultural_group.h"
#include "country/office.h"

namespace metternich {

template <typename T>
bool data_entry_compare<T>::operator()(const T *lhs, const T *rhs) const
{
	return lhs->get_identifier() < rhs->get_identifier();
}

template struct data_entry_compare<cultural_group>;
template struct data_entry_compare<named_data_entry>;
template struct data_entry_compare<office>;

}
