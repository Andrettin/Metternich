#include "metternich.h"

#include "country/ideology.h"

#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"

namespace metternich {

void ideology::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Ideology \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	data_entry::initialize();
}

void ideology::check() const
{
	assert_throw(this->get_color().isValid());
}

}
