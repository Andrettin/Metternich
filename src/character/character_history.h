#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")

namespace metternich {

class character;
class domain;
class province;
class site;

class character_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(const metternich::domain* country MEMBER domain)
	Q_PROPERTY(int level MEMBER level READ get_level)
	Q_PROPERTY(const metternich::province* deployment_province MEMBER deployment_province)

public:
	explicit character_history(const metternich::character *character);

	virtual void process_gsml_property(const gsml_property &property) override;

	const metternich::domain *get_country() const
	{
		return this->domain;
	}

	int get_level() const
	{
		return this->level;
	}

	const province *get_deployment_province() const
	{
		return this->deployment_province;
	}

private:
	const metternich::character *character = nullptr;
	const metternich::domain *domain = nullptr;
	int level = 0;
	const province *deployment_province = nullptr;
};

}
