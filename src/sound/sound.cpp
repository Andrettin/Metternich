#include "metternich.h"

#include "sound/sound.h"

#include "database/database.h"
#include "database/preferences.h"
#include "util/assert_util.h"
#include "util/path_util.h"
#include "util/vector_random_util.h"

#include <qcorosignal.h>

#include <QSoundEffect>

namespace metternich {

sound::sound(const std::string &identifier) : data_entry(identifier)
{
}

sound::~sound()
{
}

void sound::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "sounds") {
		for (const std::string &value : values) {
			this->sounds.push_back(sound::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void sound::check() const
{
	if (this->get_filepath().empty() && this->sounds.empty()) {
		throw std::runtime_error(std::format("Sound \"{}\" has neither a filepath, nor a list of other sounds to play.", this->get_identifier()));
	}

	if (!this->get_filepath().empty() && !this->sounds.empty()) {
		throw std::runtime_error(std::format("Sound \"{}\" has both a filepath and a list of other sounds to play.", this->get_identifier()));
	}
}

void sound::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_sounds_path(this->get_module()) / filepath;

	const QUrl source = QUrl::fromLocalFile(path::to_qstring(this->get_filepath()));
	this->sound_effect = make_qunique<QSoundEffect>();
	this->sound_effect->setLoopCount(1);
	this->sound_effect->setVolume(1.0f);
	this->sound_effect->setSource(source);

	connect(this->sound_effect.get(), &QSoundEffect::playingChanged, [this]() {
		if (!this->sound_effect->isPlaying()) {
			emit played();
		}
	});
}

QCoro::Task<void> sound::play_coro(const std::optional<std::chrono::milliseconds> &timeout) const
{
	if (!preferences::get()->are_sound_effects_enabled()) {
		co_return;
	}

	if (!this->sounds.empty()) {
		co_await vector::get_random_async(this->sounds)->play_coro();
		co_return;
	}

	assert_throw(this->sound_effect != nullptr);

	this->sound_effect->play();
	if (timeout.has_value()) {
		co_await qCoro(const_cast<sound *>(this), &sound::played, timeout.value());
	} else {
		co_await qCoro(const_cast<sound *>(this), &sound::played);
	}
}

}
