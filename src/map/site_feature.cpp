#include "metternich.h"

#include "map/site_feature.h"

#include "script/modifier.h"

namespace metternich {

site_feature::site_feature(const std::string &identifier) : named_data_entry(identifier)
{
}

site_feature::~site_feature()
{
}

void site_feature::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

}
