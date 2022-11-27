#include "metternich.h"

#include "country/cultural_group.h"

#include "country/cultural_group_rank.h"

namespace metternich {

cultural_group::cultural_group(const std::string &identifier)
	: culture_base(identifier), rank(cultural_group_rank::none)
{
}

void cultural_group::check() const
{
	if (this->get_rank() == cultural_group_rank::none) {
		throw std::runtime_error("Cultural group \"" + this->get_identifier() + "\" has no rank.");
	}

	if (this->get_group() != nullptr && this->get_rank() >= this->get_group()->get_rank()) {
		throw std::runtime_error("The rank of cultural group \"" + this->get_identifier() + "\" is greater than or equal to that of its upper group.");
	}
}

}
