#include "metternich.h"

#include "character/character_type.h"

#include "country/culture.h"
#include "util/assert_util.h"

namespace metternich {

void character_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
}

}
