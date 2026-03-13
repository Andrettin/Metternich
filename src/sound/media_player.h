#pragma once

#include "util/qunique_ptr.h"
#include "util/singleton.h"

class QAudioOutput;
class QMediaPlayer;

namespace metternich {

class music;

class media_player final : public QObject, public singleton<media_player>
{
	Q_OBJECT

public:
	media_player();
	~media_player();

	Q_INVOKABLE void play_music(const music *music);

private:
	qunique_ptr<QMediaPlayer> music_player;
	qunique_ptr<QAudioOutput> audio_output;
};

}
