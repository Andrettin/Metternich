#pragma once

#include "util/fractional_int.h"
#include "util/qunique_ptr.h"
#include "util/singleton.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class game_rules;

class preferences final : public QObject, public singleton<preferences>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::centesimal_int scale_factor READ get_scale_factor WRITE set_scale_factor NOTIFY scale_factor_changed)
	Q_PROPERTY(QString scale_factor_string READ get_scale_factor_qstring WRITE set_scale_factor_qstring NOTIFY scale_factor_changed)
	Q_PROPERTY(metternich::game_rules* game_rules READ get_game_rules CONSTANT)

public:
	static std::filesystem::path get_path();

	preferences();

	void load();
	void load_file();
	Q_INVOKABLE void save() const;
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const centesimal_int &get_scale_factor() const
	{
		return this->scale_factor;
	}

	void set_scale_factor(const centesimal_int &factor);

	QString get_scale_factor_qstring() const
	{
		return QString::fromStdString(this->scale_factor.to_string());
	}

	void set_scale_factor_qstring(const QString &factor_str)
	{
		this->set_scale_factor(centesimal_int(factor_str.toStdString()));
	}

	game_rules *get_game_rules()
	{
		return this->game_rules.get();
	}

	const game_rules *get_game_rules() const
	{
		return this->game_rules.get();
	}

signals:
	void scale_factor_changed();

private:
	centesimal_int scale_factor = centesimal_int(2);
	qunique_ptr<game_rules> game_rules;
};

}
