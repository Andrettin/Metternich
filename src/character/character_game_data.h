#pragma once

namespace metternich {

class character;
class country;
enum class attribute;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* employer READ get_employer_unconst NOTIFY employer_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(int primary_attribute_value READ get_primary_attribute_value NOTIFY attributes_changed)

public:
	explicit character_game_data(const metternich::character *character);

	const metternich::country *get_employer() const
	{
		return this->employer;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_employer_unconst() const
	{
		return const_cast<metternich::country *>(this->get_employer());
	}

public:
	void set_employer(const metternich::country *employer);

	int get_age() const;

	int get_attribute_value(const attribute attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end() && find_iterator->second >= 0) {
			return find_iterator->second;
		}

		return 0;
	}

	void calculate_attributes();

	int get_primary_attribute_value() const;

signals:
	void employer_changed();
	void age_changed();
	void attributes_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::country *employer = nullptr;
	std::map<attribute, int> attribute_values;
};

}
