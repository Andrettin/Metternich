#include "metternich.h"

#include "culture/cultural_group.h"

#include "culture/cultural_group_rank.h"
#include "domain/domain.h"
#include "util/vector_util.h"

namespace metternich {

cultural_group::cultural_group(const std::string &identifier)
	: culture_base(identifier), rank(cultural_group_rank::none)
{
}

void cultural_group::initialize()
{
	if (this->get_cultural_union() != nullptr) {
		for (const culture *culture : this->get_cultures()) {
			if (!vector::contains(this->get_cultural_union()->get_cultures(), culture)) {
				this->cultural_union->add_culture(culture);
			}
		}
	}

	culture_base::initialize();
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

void cultural_group::add_culture(const culture *culture)
{
	if (vector::contains(this->get_cultures(), culture)) {
		return;
	}

	this->cultures.push_back(culture);

	if (this->get_upper_group() != nullptr) {
		this->get_upper_group()->add_culture(culture);
	}
}

}
