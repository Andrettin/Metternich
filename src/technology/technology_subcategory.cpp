#include "metternich.h"

#include "technology/technology_subcategory.h"

#include "technology/technology.h"
#include "technology/technology_category.h"
#include "util/container_util.h"
#include "util/log_util.h"

namespace metternich {
	
void technology_subcategory::initialize()
{
	if (this->category != nullptr) {
		this->category->add_subcategory(this);
	}

	named_data_entry::initialize();
}
	
void technology_subcategory::check() const
{
	if (this->get_category() == nullptr) {
		throw std::runtime_error(std::format("Technology subcategory \"{}\" has no category.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Technology subcategory \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_technologies().empty()) {
		throw std::runtime_error(std::format("Technology subcategory \"{}\" has no technologies.", this->get_identifier()));
	}

	log_trace(std::format("Technology subcategory \"{}\" has {} technologies.", this->get_identifier(), this->get_technologies().size()));
}

QVariantList technology_subcategory::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

void technology_subcategory::add_technology(const technology *technology)
{
	this->technologies.push_back(technology);

	if (this->category != nullptr) {
		this->category->add_technology(technology);
	}
}

}
