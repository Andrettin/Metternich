#include "metternich.h"

#include "technology/technology.h"

#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

void technology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(technology::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void technology::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
