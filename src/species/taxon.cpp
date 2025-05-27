#include "metternich.h"

#include "species/taxon.h"

#include "species/taxonomic_rank.h"

namespace metternich {

taxon::taxon(const std::string &identifier) : taxon_base(identifier), rank(taxonomic_rank::none)
{
}

void taxon::check() const
{
	if (this->get_rank() == taxonomic_rank::none) {
		throw std::runtime_error("Taxon \"" + this->get_identifier() + "\" has no rank.");
	}

	if (this->get_rank() == taxonomic_rank::species) {
		throw std::runtime_error("Taxon \"" + this->get_identifier() + "\" has \"species\" as its taxonomic rank.");
	}

	if (this->get_supertaxon() != nullptr && this->get_rank() >= this->get_supertaxon()->get_rank()) {
		throw std::runtime_error("The rank of taxon \"" + this->get_identifier() + "\" is greater than or equal to that of its supertaxon.");
	}
}

}
