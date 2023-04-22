#include "metternich.h"

#include "population/phenotype.h"

#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"

namespace metternich {

void phenotype::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Phenotype \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	named_data_entry::initialize();
}

void phenotype::check() const
{
	assert_throw(this->get_color().isValid());
}

}
