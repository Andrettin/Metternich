#include "metternich.h"

#include "script/scripted_scoped_modifier.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "script/modifier.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
scripted_scoped_modifier<scope_type>::scripted_scoped_modifier()
{
}

template <typename scope_type>
scripted_scoped_modifier<scope_type>::~scripted_scoped_modifier()
{
}

template <typename scope_type>
bool scripted_scoped_modifier<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const scope_type>>();
		database::process_gsml_data(this->modifier, scope);
		return true;
	} else {
		return false;
	}
}

template <typename scope_type>
void scripted_scoped_modifier<scope_type>::check() const
{
	assert_throw(this->get_modifier() != nullptr);
}

template <typename scope_type>
QString scripted_scoped_modifier<scope_type>::get_modifier_string(const scope_type *scope) const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string(scope));
}

template class scripted_scoped_modifier<character>;
template class scripted_scoped_modifier<country>;
template class scripted_scoped_modifier<province>;
template class scripted_scoped_modifier<site>;

}
