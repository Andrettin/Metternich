#pragma once

#include "database/data_type.h"
#include "domain/idea_trait.h"

namespace metternich {

class deity_trait final : public idea_trait, public data_type<deity_trait>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "deity_trait";
	static constexpr const char property_class_identifier[] = "metternich::deity_trait*";
	static constexpr const char database_folder[] = "traits/deity";

	explicit deity_trait(const std::string &identifier);
	~deity_trait();

	virtual idea_type get_idea_type() const override;

signals:
	void changed();
};

}
