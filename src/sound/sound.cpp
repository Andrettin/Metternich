#include "metternich.h"

#include "sound/sound.h"

#include "database/database.h"
#include "database/preferences.h"
#include "util/assert_util.h"
#include "util/path_util.h"

#include <QSoundEffect>

namespace metternich {

sound::sound(const std::string &identifier) : data_entry(identifier)
{
	this->sound_effect = make_qunique<QSoundEffect>();
	this->sound_effect->setLoopCount(1);
	this->sound_effect->setVolume(1.0f);
}

sound::~sound()
{
}

void sound::check() const
{
	assert_throw(!this->get_filepath().empty());
}

void sound::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_sounds_path(this->get_module()) / filepath;

	const QUrl source = QUrl::fromLocalFile(path::to_qstring(this->get_filepath()));
	this->sound_effect->setSource(source);
}

void sound::play() const
{
	if (!preferences::get()->are_sound_effects_enabled()) {
		return;
	}

	if (this->sound_effect->isPlaying()) {
		return;
	}

	this->sound_effect->play();
}

}
