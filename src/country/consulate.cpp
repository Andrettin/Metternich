#include "metternich.h"

#include "country/consulate.h"

#include "util/assert_util.h"

namespace metternich {

void consulate::check() const
{
	assert_throw(this->get_level() > 0);
	assert_throw(this->get_icon() != nullptr);
}

}
