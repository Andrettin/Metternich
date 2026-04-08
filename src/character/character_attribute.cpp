#include "metternich.h"

#include "character/character_attribute.h"

#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

character_attribute::character_attribute(const std::string &identifier) : character_stat(identifier)
{
}

character_attribute::~character_attribute()
{
}

void character_attribute::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "rating_ranges") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const std::vector<std::string> rating_values = string::split(value, '-');
			assert_throw(rating_values.size() >= 1 && rating_values.size() <= 2);

			std::pair<int, int> range {};
			range.first = std::stoi(rating_values.at(0));
			if (rating_values.size() == 2) {
				range.second = std::stoi(rating_values.at(1));
			} else {
				range.second = range.first;
			}

			this->rating_ranges[key] = std::move(range);
		});
	} else {
		character_stat::process_gsml_scope(scope);
	}
}

}
