#pragma once

Q_MOC_INCLUDE("item/item_type.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class icon;
class item_slot;
class item_type;

class item final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(const metternich::item_type* type READ get_type CONSTANT)
	Q_PROPERTY(const metternich::icon* icon READ get_icon CONSTANT)

public:
	explicit item(const item_type *type);
	explicit item(const gsml_data &scope);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void set_name(const std::string &name)
	{
		if (name == this->get_name()) {
			return;
		}

		this->name = name;

		emit name_changed();
	}

	void update_name();

	const item_type *get_type() const
	{
		return this->type;
	}

	const item_slot *get_slot() const;
	const icon *get_icon() const;

	bool is_equipped() const
	{
		return this->equipped;
	}

	void set_equipped(const bool equipped)
	{
		this->equipped = equipped;
	}

signals:
	void name_changed();

private:
	std::string name;
	const item_type *type = nullptr;
	bool equipped = false;
};

}
