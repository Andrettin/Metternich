#include "metternich.h"

#include "technology/technology_category.h"

#include "util/container_util.h"
#include "util/log_util.h"

namespace metternich {
	
void technology_category::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Technology category \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_subcategories().empty()) {
		throw std::runtime_error(std::format("Technology category \"{}\" has no subcategories.", this->get_identifier()));
	}

	if (this->get_technologies().empty()) {
		throw std::runtime_error(std::format("Technology category \"{}\" has no technologies.", this->get_identifier()));
	}

	log_trace(std::format("Technology category \"{}\" has {} technologies.", this->get_identifier(), this->get_technologies().size()));
}

QVariantList technology_category::get_subcategories_qvariant_list() const
{
	return container::to_qvariant_list(this->get_subcategories());
}

QVariantList technology_category::get_technologies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_technologies());
}

}
