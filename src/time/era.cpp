#include "metternich.h"

#include "time/era.h"

#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void era::check() const
{
	assert_throw(this->get_start_date().isValid());
}

}
