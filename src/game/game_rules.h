#pragma once

#include "util/qunique_ptr.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class game_rules final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool advisors_enabled READ are_advisors_enabled WRITE set_advisors_enabled NOTIFY advisors_enabled_changed)
	Q_PROPERTY(bool myths_enabled READ are_myths_enabled WRITE set_myths_enabled NOTIFY myths_enabled_changed)

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	qunique_ptr<game_rules> duplicate() const
	{
		auto duplicate = make_qunique<game_rules>();

		duplicate->advisors_enabled = this->are_advisors_enabled();
		duplicate->myths_enabled = this->are_myths_enabled();

		return duplicate;
	}

	bool are_advisors_enabled() const
	{
		return this->advisors_enabled;
	}

	void set_advisors_enabled(const bool enabled)
	{
		if (enabled == this->are_advisors_enabled()) {
			return;
		}

		this->advisors_enabled = enabled;

		emit advisors_enabled_changed();
	}

	bool are_myths_enabled() const
	{
		return this->myths_enabled;
	}

	void set_myths_enabled(const bool enabled)
	{
		if (enabled == this->are_myths_enabled()) {
			return;
		}

		this->myths_enabled = enabled;

		emit myths_enabled_changed();
	}

signals:
	void advisors_enabled_changed();
	void myths_enabled_changed();

private:
	bool advisors_enabled = true;
	bool myths_enabled = true;
};

}
