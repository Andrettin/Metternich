#include "metternich.h"

#include "sound/media_player.h"

#include "database/preferences.h"
#include "sound/music.h"
#include "util/assert_util.h"
#include "util/path_util.h"

#include <QAudioOutput>
#include <QMediaPlayer>

namespace metternich {

media_player::media_player()
{
	this->music_player = make_qunique<QMediaPlayer>();
	this->audio_output = make_qunique<QAudioOutput>();
	this->music_player->setAudioOutput(this->audio_output.get());
	this->audio_output->setVolume(0.5f);
}

media_player::~media_player()
{
}

void media_player::play_music(const music *music)
{
	assert_throw(music != nullptr);

	if (!preferences::get()->is_music_enabled()) {
		return;
	}

	const QUrl source = QUrl::fromLocalFile(path::to_qstring(music->get_filepath()));
	this->music_player->setSource(source);
	this->music_player->play();
}

void media_player::stop_music()
{
	if (this->music_player->isPlaying()) {
		this->music_player->stop();
	}
}

}
