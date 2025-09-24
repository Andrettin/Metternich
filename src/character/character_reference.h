#pragma once

namespace metternich {

class character;

class character_reference final
{
public:
	explicit character_reference(metternich::character *character);
	~character_reference();

	const metternich::character *get_character() const
	{
		return this->character;
	}

private:
	metternich::character *character = nullptr;
};

}
