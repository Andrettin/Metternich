import QtQuick
import QtQuick.Controls
import ".."

MenuBase {
	id: game_rules_menu
	title: qsTr("Options")
	
	property bool options_changed: false
	
	Grid {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.top: title_item.bottom
		anchors.topMargin: 32 * scale_factor
		anchors.bottom: previous_menu_button.top
		anchors.bottomMargin: 32 * scale_factor
		spacing: 16 * scale_factor
		
		CustomCheckBox {
			id: sound_effects_enabled_checkbox
			text: qsTr("Sound Effects Enabled")
			checked: metternich.preferences.sound_effects_enabled
			checkable: true
			onCheckedChanged: {
				if (metternich.preferences.sound_effects_enabled !== checked) {
					metternich.preferences.sound_effects_enabled = checked
					options_changed = true
				}
			}
			
		}
		
		CustomCheckBox {
			id: music_enabled_checkbox
			text: qsTr("Music Enabled")
			checked: metternich.preferences.music_enabled
			checkable: true
			onCheckedChanged: {
				if (metternich.preferences.music_enabled !== checked) {
					metternich.preferences.music_enabled = checked
					options_changed = true
					
					if (metternich.preferences.music_enabled) {
						if (metternich.defines.main_menu_music !== null) {
							metternich.media_player.play_music(metternich.defines.main_menu_music)
						}
					}
				}
			}
			
		}
	}
	
	TextButton {
		id: previous_menu_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 16 * scale_factor
		text: qsTr("Previous Menu")
		width: 96 * scale_factor
		height: 24 * scale_factor
		
		onClicked: {
			if (options_changed) {
				metternich.preferences.save()
			}
			menu_stack.pop()
		}
	}
}
