#include "metternich.h"

#include "technology/technology.h"

#include "util/assert_util.h"

namespace metternich {

void technology::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
