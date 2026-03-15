import QtQuick
import QtQuick.Controls
import ".."
import "../dialogs"

MenuBase {
	id: main_menu
	title: qsTr("Main Menu")
	
	Column {
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: play_scenario_button
			text: qsTr("Play Scenario")
			width: 128 * scale_factor
			
			onClicked: {
				menu_stack.push("ScenarioMenu.qml")
			}
		}
		
		TextButton {
			id: load_game_button
			text: "Load Game"
			width: 128 * scale_factor
			onClicked: {
				load_game_dialog.open()
			}
		}
		
		TextButton {
			id: options_button
			text: qsTr("Options")
			width: 128 * scale_factor
			
			onClicked: {
				menu_stack.push("OptionsMenu.qml")
			}
		}
		
		TextButton {
			id: exit_button
			text: qsTr("Exit")
			width: 128 * scale_factor
			
			onClicked: {
				window.close()
			}
		}
	}
	
	LoadGameDialog {
		id: load_game_dialog
	}
	
	Connections {
		target: metternich
		function onRunningChanged() {
			if (metternich.running) {
				if (metternich.defines.main_menu_music !== null) {
					metternich.media_player.play_music(metternich.defines.main_menu_music)
				}
			}
		}
	}
}
