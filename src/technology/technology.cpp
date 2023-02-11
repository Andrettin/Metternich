#include "metternich.h"

#include "technology/technology.h"

#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

technology::technology(const std::string &identifier) : named_data_entry(identifier)
{
}

technology::~technology()
{
}

void technology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(technology::get(value));
		}
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void technology::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

QVariantList technology::get_prerequisites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_prerequisites());
}

bool technology::requires_technology(const technology *technology) const
{
	assert_throw(this != technology);

	for (const metternich::technology *prerequisite : this->get_prerequisites()) {
		if (prerequisite == technology || prerequisite->requires_technology(technology)) {
			return true;
		}
	}

	return false;
}

int technology::get_total_prerequisite_depth() const
{
	int depth = 0;

	for (const technology *prerequisite : this->get_prerequisites()) {
		depth = std::max(depth, prerequisite->get_total_prerequisite_depth() + 1);
	}

	return depth;
}

QString technology::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string());
}

}
